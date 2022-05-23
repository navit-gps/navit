##################################################################
## EntitiesV3.pm - Wraps entities used by OSM                   ##
## By Martijn van Oosterhout <kleptog@svana.org>                ##
##                                                              ##
## Package that wraps the entities used by OSM into Perl        ##
## object, so they can be easily manipulated by various         ##
## packages.                                                    ##
##                                                              ##
## Licence: LGPL                                                ##
##################################################################

use XML::Writer;
use strict;

############################################################################
## Top level Entity type, parent of all types, includes stuff relating to ##
## tags and IDs which are shared by all entity types                      ##
############################################################################
package Geo::OSM::Entity;
use POSIX qw(strftime);

use Carp;

sub _new
{
  bless {}, shift;
}

sub _get_writer
{
  my($self,$res) = @_;
  return new XML::Writer(OUTPUT => $res, NEWLINES => 0, ENCODING => 'utf-8');
}

sub add_tag
{
  my($self, $k,$v) = @_;
  push @{$self->{tags}}, $k, $v;
}

sub add_tags
{
  my($self, @tags) = @_;
  if( scalar(@tags)&1 )
  { croak "add_tags requires an even number of arguments" }
  push @{$self->{tags}}, @tags;
}

sub set_tags
{
  my($self,$tags) = @_;
  if( ref($tags) eq "HASH" )
  { $self->{tags} = [%$tags] }
  elsif( ref($tags) eq "ARRAY" )
  { $self->{tags} = [@$tags] }
  else
  { croak "set_tags must be HASH or ARRAY" }
}

sub tags
{
  my $self = shift;
  return [@{$self->{tags}}];  # Return copy
}

sub tag_xml
{
  my ($self,$writer) = @_;
  my @a = @{$self->{tags}};

  my $str = "";

  while( my($k,$v) = splice @a, 0, 2 )
  {
    $writer->emptyTag( "tag", 'k' => $k, 'v' => $v );
  }
}

our $_ID = -1;

sub set_id
{
  my($self,$id) = @_;

  if( not defined $id )
  { $id = $_ID-- }
  $self->{id} = $id;
}

sub id
{
  my $self = shift;
  return $self->{id};
}

sub set_timestamp
{
  my($self,$time) = @_;
  if( defined $time )
  { $self->{timestamp} = $time }
  else
  { $self->{timestamp} = strftime "%Y-%m-%dT%H:%M:%S+00:00", gmtime(time) }
}

sub timestamp
{
  my $self = shift;
  return $self->{timestamp};
}

sub full_xml
{
  my $self = shift;
  return qq(<?xml version="1.0"?>\n<osm version="0.5">\n).$self->xml()."</osm>\n";
}

package Geo::OSM::Way;
our @ISA = qw(Geo::OSM::Entity);
use Carp;

sub new
{
  my($class, $attr, $tags, $nodes) = @_;

  my $obj = bless $class->SUPER::_new(), $class;

  $obj->set_tags($tags);
  $obj->set_nodes($nodes);
  $obj->set_id($attr->{id} );
  $obj->set_timestamp( $attr->{timestamp} );

  return $obj;
}

sub type { return "way" }

sub set_nodes
{
  my($self,$nodes) = @_;
  if( not defined $nodes )
  { $nodes = [] }
  if( ref($nodes) ne "ARRAY" )
  { $nodes = [$nodes] }
  if( scalar( grep { (ref($_) ne "")?$_->type ne "node":$_ !~ /^-?\d+/ } @$nodes ) )
  { croak "Expected array of nodes" }
  $self->{nodes} = [map { ref($_)?$_->id:$_ } @$nodes];
}

sub nodes
{
  my $self = shift;
  return [@{$self->{nodes}}];  # Return a copy
}

sub xml
{
  my $self = shift;
  my $str = "";
  my $writer = $self->_get_writer(\$str);

  $writer->startTag( "way", id => $self->id, timestamp => $self->timestamp );
  $self->tag_xml( $writer );
  for my $node (@{$self->nodes})
  {
    $writer->emptyTag( "nd", ref => $node );
  }
  $writer->endTag( "way" );
  $writer->end;
  return $str;
}

sub map
{
  my($self,$mapper) = @_;
  my $incomplete = 0;
  my ($new_id) = $mapper->map('way',$self->id);   # Determine mapped ID
  # It is ok for the new_id to be incomplete; it may be a create request

  my @new_nodes = map { [ $mapper->map('node',$_) ] } @{$self->nodes};
  map { $incomplete |= $_->[1] } @new_nodes;
  # incomplete tracks if any of the segs are incomplete

  my $new_ent = new Geo::OSM::Way( {id=>$new_id, timestamp=>$self->timestamp}, $self->tags, [map {$_->[0]} @new_nodes] );
  return($new_ent,$incomplete);
}

package Geo::OSM::Relation::Member;
use Carp;
# Relation reference can be specified in several ways:
# { type => $type, ref => $id [ , role => $str ] }
# { ref => $obj [ , role => $str ] }
# [ $type, $id [,$role] ]
# [ $obj, [,$role] ]
sub new
{
  my $class = shift;
  my $arg = shift;
  return $arg if ref($arg) eq $class;   # Return if already object
  if( ref($arg) eq "ARRAY" )
  {
    if( ref $arg->[0] )
    { $arg = { ref => $arg->[0], role => $arg->[1] } }
    else
    { $arg = { type => $arg->[0], ref => $arg->[1], role => $arg->[2] } }
  }
  if( ref($arg) eq "HASH" )
  {
    if( ref $arg->{ref} )
    { $arg = [ $arg->{ref}->type, $arg->{ref}->id, $arg->{role} ] }
    else
    { $arg = [ $arg->{type}, $arg->{ref}, $arg->{role} ] }
  }
  else
  { croak "Relation reference must be array or hash" }
  croak "Bad type of member '$arg->[0]'"
    unless $arg->[0] =~ /^(way|node|relation)$/;
  croak "Bad member id '$arg->[1]'"
    unless $arg->[1] =~ /^-?\d+$/;
  $arg->[2] ||= "";

  return bless $arg, $class;
}

sub member_type { shift->[0] }
sub ref { shift->[1] }
sub role { shift->[2] }

sub type { return "relation:member" }

sub _xml
{
  my $self = shift;
  my $writer = shift;

  $writer->emptyTag( "member", type => $self->member_type, ref => $self->ref, role => $self->role );
}

sub map
{
  my($self,$mapper) = @_;
  my ($new_ref,$incomplete) = $mapper->map($self->member_type,$self->ref);
  my $new_member = new Geo::OSM::Relation::Member( { type => $self->member_type, ref => $new_ref, role => $self->role } );
  return($new_member,$incomplete);
}

package Geo::OSM::Relation;
our @ISA = qw(Geo::OSM::Entity);

sub new
{
  my($class, $attr, $tags, $members) = @_;

  my $obj = bless $class->SUPER::_new(), $class;

  $obj->set_tags($tags);
  $obj->set_members($members);
  $obj->set_id($attr->{id} );
  $obj->set_timestamp( $attr->{timestamp} );

  return $obj;
}

sub set_members
{
  my($self,$members) = @_;
  if( not defined $members )
  { $members = [] }
  if( ref($members) ne "ARRAY" )
  { $members = [$members] }
  $self->{members} = [map { new Geo::OSM::Relation::Member($_) } @$members];
}

sub members
{
  my $self = shift;
  return [@{$self->{members}}];
}

sub type { return "relation" }

sub xml
{
  my $self = shift;
  my $str = "";
  my $writer = $self->_get_writer(\$str);

  $writer->startTag( "relation", id => $self->id, timestamp => $self->timestamp );
  $self->tag_xml( $writer );
  # Write members
  foreach my $member (@{$self->{members}})
  { $member->_xml( $writer ) }
  $writer->endTag( "relation" );
  $writer->end;
  return $str;
}

sub map
{
  my($self,$mapper) = @_;
  my $incomplete = 0;

  my ($new_id) = $mapper->map('relation',$self->id);
  my @new_members = map { [ $_->map($mapper) ] } @{$self->members};
  map { $incomplete |= $_->[1] } @new_members;
  # incomplete tracks if any of the members are incomplete
  my $new_ent = new Geo::OSM::Relation( {id=>$new_id, timestamp=>$self->timestamp}, $self->tags, [map {$_->[0]} @new_members] );
  return($new_ent,$incomplete);
}

package Geo::OSM::Node;
use Carp;
our @ISA = qw(Geo::OSM::Entity);

sub new
{
  my($class, $attr, $tags) = @_;

  my $obj = bless $class->SUPER::_new(), $class;

  $obj->set_tags($tags);
  $obj->set_id($attr->{id} );
  $obj->set_timestamp( $attr->{timestamp} );
  if( $attr->{lon} !~ /^[-+]?\d+(\.\d+)?([eE][+-]?\d+)?$/ or
      $attr->{lat} !~ /^[-+]?\d+(\.\d+)?([eE][+-]?\d+)?$/ )
  {
    croak "Invalid lat,lon values ($attr->{lat},$attr->{lon})\n";
  }
  $obj->{lon} = $attr->{lon};
  $obj->{lat} = $attr->{lat};

  return $obj;
}

sub type { return "node" }

sub set_latlon
{
  my($self,$lat,$lon) = @_;
  $self->{lat} = $lat;
  $self->{lon} = $lon;
}

sub lat
{
  shift->{lat};
}
sub lon
{
  shift->{lon};
}

sub xml
{
  my $self = shift;
  my $str = "";
  my $writer = $self->_get_writer(\$str);

  $writer->startTag( "node", id => $self->id, lat => $self->lat, lon => $self->lon, timestamp => $self->timestamp );
  $self->tag_xml( $writer );
  $writer->endTag( "node" );
  $writer->end;
  return $str;
}

sub map
{
  my($self,$mapper) = @_;
  my ($new_id) = $mapper->map('node',$self->id);
  my $new_ent = new Geo::OSM::Node( {id=>$new_id, timestamp=>$self->timestamp, lat=>$self->lat, lon=>$self->lon}, $self->tags );
  return($new_ent,0);
}



1;
