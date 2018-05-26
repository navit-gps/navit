##################################################################
## OsmReader.pm - Library for reading OSM  files                ##
## By Martijn van Oosterhout <kleptog@svana.org>                ##
##                                                              ##
## Package that reads both OSM file format files.               ##
## The user creates the parse with a callback and the           ##
## loader will call the callback for each detected object.      ##
## Licence: LGPL                                                ##
##################################################################
package Geo::OSM::OsmReader;

use strict;
use warnings;

use Utils::File;
use Utils::Math;
use Utils::Debug;
use XML::Parser;
use Carp;

use Geo::OSM::EntitiesV5;

use constant STATE_INIT => 1;
use constant STATE_EXPECT_ENTITY => 3;
use constant STATE_EXPECT_BODY => 4;

# With this initialiser, your process will get called with instantiated objects
sub init
{
  my $obj = bless{}, shift;
  my $proc = shift;
  my $prog = shift;
  if( ref $proc ne "CODE" )
  { die "init Geo::OSM::OsmReader requires a sub as argument\n" }
  $obj->{newproc}  = $proc;
  if( defined $prog )
  { $obj->{progress} = $prog }
  return $obj;
}

sub _process
{
  my($self, $entity, $attr, $tags, $members) = @_;

  my $ent;
  if( $entity eq "node" )
  {
    $ent = new Geo::OSM::Node( $attr, $tags );
  }
  if( $entity eq "relation" )
  {
    $ent = new Geo::OSM::Relation( $attr, $tags, $members );
  }
  if( $entity eq "way" )
  {
    $ent = new Geo::OSM::Way( $attr, $tags, $members );
  }
  croak "Unknown entity '$entity'" if not defined $ent;

  return $self->{newproc}->( $ent );
}

sub load($)
{
  my ($self, $file_name) = @_;

  $self->{state} = STATE_INIT;

  my $start_time = time();
  my $P = new XML::Parser(Handlers => {Start => sub{ DoStart( $self, @_ )}, End => sub { DoEnd( $self, @_ )}});
    my $fh = data_open($file_name);
    die "Cannot open OSM File $file_name\n" unless $fh;
    $self->{input_length} = -s $fh;
    $self->{count}=0;
    eval {
	$P->parse($fh);
    };
    print "\n" if $DEBUG || $VERBOSE;
    if ( $VERBOSE) {
	printf "Read and parsed $file_name in %.0f sec\n",time()-$start_time;
    }
    if ( $@ ) {
	warn "$@Error while parsing\n $file_name\n";
	return;
    }
    if (not $P) {
	warn "WARNING: Could not parse osm data\n";
	return;
    }
}

sub parse($)
{
  my ($self, $string) = @_;

  $self->{state} = STATE_INIT;

  my $start_time = time();
  my $P = new XML::Parser(Handlers => {Start => sub{ DoStart( $self, @_ )}, End => sub { DoEnd( $self, @_ )}});
    $self->{input_length} = length($string);
    $self->{count}=0;
    eval {
	$P->parse($string);
    };
    print "\n" if $DEBUG || $VERBOSE;
    if ( $VERBOSE) {
	printf "Read and parsed string in %.0f sec\n",time()-$start_time;
    }
    if ( $@ ) {
	warn "$@Error while parsing\n [$string]\n";
	return;
    }
    if (not $P) {
	warn "WARNING: Could not parse osm data\n";
	return;
    }
}

# Function is called whenever an XML tag is started
sub DoStart
{
#print @_,"\n";
  my ($self, $Expat, $Name, %Attr) = @_;

  if( $self->{state} == STATE_INIT )
  {
    if($Name eq "osm"){
      $self->{state} = STATE_EXPECT_ENTITY;

      if( $Attr{version} ne "0.5" )
      { die "OsmReaderV5 can only read 0.5 files, found '$Attr{version}'\n" }
    } else {
      die "Expected 'osm' tag, got '$Name'\n";
    }
  }
  elsif( $self->{state} == STATE_EXPECT_ENTITY )
  {
    # Pick up the origin attribute from the bound tag
    if( $Name eq "bound" )
    {
      if( exists $Attr{origin} )
      {
        $self->{origin} = $Attr{origin};
      }
      return;
    }
    if($Name eq "node" or $Name eq "relation" or $Name eq "way"){
      $self->{entity} = $Name;
      $self->{attr} = {%Attr};
      $self->{tags} = [];
      $self->{members} = ($Name ne "node") ? [] : undef;
      $self->{state} = STATE_EXPECT_BODY;
    } else {
      die "Expected entity\n";
    }
  }
  elsif( $self->{state} == STATE_EXPECT_BODY )
  {
    if($Name eq "tag"){
      push @{$self->{tags}}, $Attr{"k"}, $Attr{"v"};
    }
    if($Name eq "nd"){
      push @{$self->{members}}, $Attr{"ref"};
    }
    if($Name eq "member"){
      push @{$self->{members}}, new Geo::OSM::Relation::Member( \%Attr );
    }
  }
}

# Function is called whenever an XML tag is ended
sub DoEnd
{
  my ($self, $Expat, $Name) = @_;
  if( $self->{state} == STATE_EXPECT_BODY )
  {
    if( $Name eq $self->{entity} )
    {
      $self->_process( $self->{entity}, $self->{attr}, $self->{tags}, $self->{members} );
      $self->{count}++;
      if( $self->{progress} and ($self->{count}%11) == 1)
      {
        $self->{progress}->($self->{count}, $Expat->current_byte()/$self->{input_length} );
      }
      $self->{state} = STATE_EXPECT_ENTITY;
    }
    return;
  }
  elsif( $self->{state} == STATE_EXPECT_ENTITY )
  {
    return if $Name eq "bound";
    if( $Name eq "osm" )
    {
      $self->{state} = STATE_INIT;
    } else {die}
    return;
  }
}

1;

__END__

=head1 NAME

OsmReaderV5 - Module for reading OpenStreetMap V5 XML data files

=head1 SYNOPSIS

  my $OSM = new Geo::OSM::OsmReader(\&process);
  $OSM->load("Data/changes.osc");

  sub process
  {
    my($OSM, $entity) = @_;
    print "Read $entity ".$entity->id."\n";
    my $tags = $entity->tags;
    while( my($k,$v) = splice @{$tags}, 0, 2 )
    { print "  $k: $v\n" }
    if( $entity->type eq "way" )
    { print "  Nodes: ", join(", ",@{$entity->nodes}),"\n"; }
  }

=head1 AUTHOR

Martijn van Oosterhout <kleptog@svana.org>
based on OsmXML.pm written by:
Oliver White (oliver.white@blibbleblobble.co.uk)

=head1 COPYRIGHT

Copyright 2007, Martijn van Oosterhout
Copyright 2006, Oliver White

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

=cut
