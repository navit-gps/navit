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
  return qq(<?xml version="1.0"?>\n<osm version="0.4">\n).$self->xml()."</osm>\n";
}

package Geo::OSM::Way;
our @ISA = qw(Geo::OSM::Entity);

sub new
{
  my($class, $attr, $tags, $segs) = @_;

  my $obj = bless $class->SUPER::_new(), $class;

  $obj->set_tags($tags);
  $obj->set_segs($segs);
  $obj->set_id($attr->{id} );
  $obj->set_timestamp( $attr->{timestamp} );

  return $obj;
}

sub type { return "way" }

sub set_segs
{
  my($self,$segs) = @_;
  $self->{segs} = $segs;
}

sub segs
{
  my $self = shift;
  return [@{$self->{segs}}];  # Return a copy
}

sub xml
{
  my $self = shift;
  my $str = "";
  my $writer = $self->_get_writer(\$str);

  $writer->startTag( "way", id => $self->id, timestamp => $self->timestamp );
  $self->tag_xml( $writer );
  for my $seg (@{$self->segs})
  {
    $writer->emptyTag( "seg", id => $seg );
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

  my @new_segs = map { [ $mapper->map('segment',$_) ] } @{$self->segs};
  map { $incomplete |= $_->[1] } @new_segs;
  # incomplete tracks if any of the segs are incomplete

  my $new_ent = new Geo::OSM::Way( {id=>$new_id, timestamp=>$self->timestamp}, $self->tags, [map {$_->[0]} @new_segs] );
  return($new_ent,$incomplete);
}

package Geo::OSM::Segment;
our @ISA = qw(Geo::OSM::Entity);

sub new
{
  my($class, $attr, $tags) = @_;

  my $obj = bless $class->SUPER::_new(), $class;

  $obj->set_tags($tags);
  $obj->set_id($attr->{id} );
  $obj->set_timestamp( $attr->{timestamp} );
  $obj->{from} = $attr->{from};
  $obj->{to} = $attr->{to};

  return $obj;
}

sub type { return "segment" }

sub set_fromto
{
  my($self,$from,$to) = @_;
  $self->{from} = $from;
  $self->{to} = $to;
}

sub from
{
  shift->{from};
}
sub to
{
  shift->{to};
}

sub xml
{
  my $self = shift;
  my $str = "";
  my $writer = $self->_get_writer(\$str);

  $writer->startTag( "segment", id => $self->id, from => $self->from, to => $self->to, timestamp => $self->timestamp );
  $self->tag_xml( $writer );
  $writer->endTag( "segment" );
  $writer->end;
  return $str;
}

sub map
{
  my($self,$mapper) = @_;
  my ($new_id) = $mapper->map('segment',$self->id);
  my ($new_from,$i1) = $mapper->map('node',$self->from);
  my ($new_to,$i2) = $mapper->map('node',$self->to);
  my $new_ent = new Geo::OSM::Segment( {id=>$new_id, timestamp=>$self->timestamp, from=>$new_from, to=>$new_to}, $self->tags );
  return($new_ent,$i1|$i2);
}

package Geo::OSM::Node;
our @ISA = qw(Geo::OSM::Entity);

sub new
{
  my($class, $attr, $tags) = @_;

  my $obj = bless $class->SUPER::_new(), $class;

  $obj->set_tags($tags);
  $obj->set_id($attr->{id} );
  $obj->set_timestamp( $attr->{timestamp} );
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
