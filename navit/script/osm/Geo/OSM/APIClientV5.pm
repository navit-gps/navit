##################################################################
## APIClientV5.pm - General Perl client for the API             ##
## By Martijn van Oosterhout <kleptog@svana.org>                ##
##                                                              ##
## Currently only supports uploading. Note the package actually ##
## creates a package named Geo::OSM::APIClient so upgrades to   ##
## later versions will be easier.                               ##
## Licence: LGPL                                                ##
##################################################################

use LWP::UserAgent;
use strict;

package Geo::OSM::APIClient;
use Geo::OSM::OsmReaderV5;
use MIME::Base64;
use HTTP::Request;
use Carp;
use Encode;
use POSIX qw(sigprocmask);
use URI;
use Socket qw(inet_ntoa);

sub new
{
  my( $class, %attr ) = @_;

  my $obj = bless {}, $class;

  my $url = $attr{api};
  if( not defined $url )
  {
    croak "Did not specify api url";
  }

  $url =~ s,/$,,;   # Strip trailing slash
  $obj->{url} = URI->new($url);
  $obj->{client} = new LWP::UserAgent(agent => 'Geo::OSM::APIClientV5', timeout => 1200);

  if( defined $attr{username} and defined $attr{password} )
  {
    my $encoded = MIME::Base64::encode_base64("$attr{username}:$attr{password}","");
    $obj->{client}->default_header( "Authorization", "Basic $encoded" );
  }

  # We had the problem of the client doing a DNS lookup each request. To
  # solve this we do a gethostbyname now and store that in the URI.
  {
    my $addr;
    (undef, undef, undef, undef, $addr) = gethostbyname($obj->{url}->host);
    if( defined $addr )
    {
      $obj->{client}->default_header( "Host", $obj->{url}->host );
      $obj->{url}->host( inet_ntoa($addr) );
      print STDERR "Using address: ".$obj->{url}->as_string()."\n";
    }
  }
  # Hack to avoid protocol lookups each time
  @LWP::Protocol::http::EXTRA_SOCK_OPTS = ( 'Proto' => 6 );

  $obj->{reader} = init Geo::OSM::OsmReader( sub { _process($obj,@_) } );
  return $obj;
}

# This is the callback from the parser. If checks if the buffer is defined.
# If the buffer is an array, append the new object. If the buffer is a proc,
# call it.
sub _process
{
  my($obj,$ent) = @_;
  if( not defined $obj->{buffer} )
  { die "Internal error: Received object with buffer" }
  if( ref $obj->{buffer} eq "ARRAY" )
  { push @{$obj->{buffer}}, $ent; return }
  if( ref $obj->{buffer} eq "CODE" )
  { $obj->{buffer}->($ent); return }
  die "Internal error: don't know what to do with buffer $obj->{buffer}";
}

# Utility function to handle the temporary blocking of signals in a way that
# works with exception handling.
sub _with_blocked_sigs(&)
{
  my $ss = new POSIX::SigSet( &POSIX::SIGINT );
  my $func = shift;
  my $os = new POSIX::SigSet;
  sigprocmask( &POSIX::SIG_BLOCK, $ss, $os );
  my $ret = eval { &$func };
  sigprocmask( &POSIX::SIG_SETMASK, $os );
  die $@ if $@;
  return $ret;
}

sub _request
{
  my $self = shift;
  my $req = shift;
  return _with_blocked_sigs { $self->{client}->request($req) };
}

sub last_error_code
{
  my $self=shift;
  croak "No last error" unless defined $self->{last_error};
  return $self->{last_error}->code;
}

sub last_error_message
{
  my $self=shift;
  croak "No last error" unless defined $self->{last_error};
  return $self->{last_error}->message;
}

sub create($)
{
  my( $self, $ent ) = @_;
  my $oldid = $ent->id;
  $ent->set_id(0);
  my $content = encode("utf-8", $ent->full_xml);
  $ent->set_id($oldid);
  my $req = new HTTP::Request PUT => $self->{url}."/".$ent->type()."/create";
  $req->content($content);

#  print $req->as_string;

  my $res = $self->_request($req);

#  print $res->as_string;

  if( $res->code == 200 )
  {
    return $res->content
  }

  $self->{last_error} = $res;
  return undef;
}

sub modify($)
{
  my( $self, $ent ) = @_;
  my $content = encode("utf-8", $ent->full_xml);
  my $req = new HTTP::Request PUT => $self->{url}."/".$ent->type()."/".$ent->id();
  $req->content($content);

#  print $req->as_string;

  my $res = $self->_request($req);

  return $ent->id() if $res->code == 200;
  $self->{last_error} = $res;
  return undef;
}

sub delete($)
{
  my( $self, $ent ) = @_;
  my $content = encode("utf-8", $ent->full_xml);
  my $req = new HTTP::Request DELETE => $self->{url}."/".$ent->type()."/".$ent->id();
#  $req->content($content);

#  print $req->as_string;

  my $res = $self->_request($req);

  return $ent->id() if $res->code == 200;
  $self->{last_error} = $res;
  return undef;
}

sub get($$)
{
  my $self = shift;
  my $type = shift;
  my $id = shift;
  my $extra = shift;

  $extra = "/".$extra if (defined $extra);
  $extra = "" if not defined $extra;

  my $req = new HTTP::Request GET => $self->{url}."/$type/$id$extra";

  my $res = $self->_request($req);

  if( $res->code != 200 )
  {
    $self->{last_error} = $res;
    return undef;
  }

  my @res;
  $self->{buffer} = \@res;
  $self->{reader}->parse($res->content);
  undef $self->{buffer};
  if($extra =~ /history/)
  {
    return @res;
  }
  if(scalar(@res) != 1 )
  {
    die "Unexpected response for get_$type [".$res->content()."]\n";
  }

  return $res[0];
}

sub resurrect($$)
{
  my $self = shift;
  my $type = shift;
  my $id = shift;

  my $ret = $self->get($type, $id);
  if (defined $ret || !defined $self->{last_error} || ($self->{last_error}->code != 410)) {
    return $ret;
  }

  my @ents = $self->get($type, $id, 'history');
  # we want the last _visible_ one
  my $ent = $ents[-2];
  if ($ent->type eq 'way') {
  	printf("resurrecting way, checking all member nodes...\n");
	foreach my $node_id (@{$ent->nodes()}) {
		printf("checking node: $node_id...");
		my $node_ent = $self->get('node', $node_id);
		if (defined $node_ent) {
			printf("good\n");
			next;
		}
		printf("attempting to resurrect node: $node_id...");
		$node_ent = $self->resurrect('node', $node_id);
		if (!defined $node_ent) {
			die "failed";
		}
		printf("success!\n");
	}
  	printf("all way nodes are OK, ");
  }
  printf("attempting to resurrect %s...", $ent->type);
  $ret = $self->modify($ent);
  if ($ret == $ent->id) {
	printf("ok\n");
	return $ret;
  }
  die sprintf("unable to resurrect $type $id: %s\n", $self->last_error_message);
}

sub get_node($)
{
  my $self = shift;
  return $self->get("node",shift);
}

sub get_way($)
{
  my $self = shift;
  return $self->get("way",shift);
}

sub get_relation($)
{
  my $self = shift;
  return $self->get("relation",shift);
}

sub get_subtype($$)
{
  my $self = shift;
  my $type = shift;
  my $id = shift;
  my $subtype = shift;

  my $req = new HTTP::Request GET => $self->{url}."/$type/$id/$subtype";

  my $res = $self->_request($req);

  if( $res->code != 200 )
  {
    $self->{last_error} = $res;
    return undef;
  }

  my @res;
  $self->{buffer} = \@res;
  $self->{reader}->parse($res->content);
  undef $self->{buffer};
  if( scalar(@res) < 1 )
  {
    die "Unexpected response for get_subtype($type,$id,$subtype) [".$res->content()."]\n";
  }

  return \@res;
}

sub get_node_ways($)
{
  my $self = shift;
  my $id = shift;

  return $self->get_subtype("node",$id,"ways");
}

sub map($$$$)
{
  my $self = shift;
  my @bbox = @_;

  my $req = new HTTP::Request GET => $self->{url}."/map?bbox=$bbox[0],$bbox[1],$bbox[2],$bbox[3]";

  my $res = $self->_request($req);

  if( $res->code != 200 )
  {
    $self->{last_error} = $res;
    return undef;
  }

  my @res;
  $self->{buffer} = \@res;
  $self->{reader}->parse($res->content);
  undef $self->{buffer};

  return \@res;
}

1;
