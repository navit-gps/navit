##################################################################
## APIClientV4.pm - General Perl client for the API             ##
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
use Geo::OSM::OsmReaderV3;
use MIME::Base64;
use HTTP::Request;
use Carp;
use Encode;
use POSIX qw(sigprocmask);

sub new
{
  my( $class, %attr ) = @_;

  my $obj = bless {}, $class;

  my $url = $attr{api};
  if( not defined $url )
  {
    croak "Did not specify aip url";
  }

  $url =~ s,/$,,;   # Strip trailing slash
  $obj->{url} = $url;
  $obj->{client} = new LWP::UserAgent(agent => 'Geo::OSM::APIClientV4', timeout => 1200);

  if( defined $attr{username} and defined $attr{password} )
  {
    if( $obj->{url} =~ m,http://([\w.]+)/, )
    {
      $obj->{client}->credentials( "$1:80", "Web Password",  $attr{username}, $attr{password} );
    }
    my $encoded = MIME::Base64::encode_base64("$attr{username}:$attr{password}","");
    $obj->{client}->default_header( "Authorization", "Basic $encoded" );
  }

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
  return shift->{last_error}->code;
}

sub last_error_message
{
  return shift->{last_error}->message;
}

sub create
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

sub modify
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

sub delete
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

  my $req = new HTTP::Request GET => $self->{url}."/$type/$id";

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
  if( scalar(@res) != 1 )
  {
    die "Unexpected response for get_$type [".$res->content()."]\n";
  }

  return $res[0];
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

sub get_segment($)
{
  my $self = shift;
  return $self->get("segment",shift);
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
