##################################################################
## OsmChangeReader.pm - Library for reading OSM change files    ##
## By Martijn van Oosterhout <kleptog@svana.org>                ##
##                                                              ##
## Package that reads both osmChange and JOSM file format change##
## files. The user creates the parse with a callback and the    ##
## loader will call the callback for each detected change. Note ##
## that for JOSM file entires that are not changes are ignored. ##
##                                                              ##
## Licence: LGPL                                                ##
##################################################################
package Geo::OSM::OsmChangeReader;

use strict;
use warnings;

use Utils::File;
use Utils::Math;
use Utils::Debug;
use XML::Parser;
use Carp;

use Geo::OSM::EntitiesV6;

use constant STATE_INIT => 1;
use constant STATE_EXPECT_COMMAND => 2;
use constant STATE_EXPECT_ENTITY => 3;
use constant STATE_EXPECT_BODY => 4;

use constant FILETYPE_UNKNOWN   => 0;
use constant FILETYPE_OSMCHANGE => 1;
use constant FILETYPE_OSM       => 2;

# With this initialiser, your process will get called with instantiated objects
sub init
{
  my $obj = bless{}, shift;
  my $proc = shift;
  my $prog = shift;
  if( ref $proc ne "CODE" )
  { die "init Geo::OSM::OsmChangeReader requires a sub as argument\n" }
  $obj->{newproc}  = $proc;
  if( defined $prog )
  { $obj->{progress} = $prog }
  return $obj;
}

sub _process
{
  my($self, $command, $entity, $attr, $tags, $members) = @_;

  if( defined $self->{oldproc} )
  {
    return $self->{oldproc}->($command, $entity, $attr, $tags, $members);
  }

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

  return $self->{newproc}->($command, $ent );
}

sub load{
  my ($self, $file_name) = @_;

  $self->{filetype} = FILETYPE_UNKNOWN;
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

  if( $self->{filetype} == FILETYPE_UNKNOWN )
  {
    if( $self->{state} == STATE_INIT )
    {
      if($Name eq "osmChange"){
        $self->{state} = STATE_EXPECT_COMMAND;
        $self->{filetype} = FILETYPE_OSMCHANGE;

        if( $Attr{version} ne "0.6" )
        { die "OsmChangeReaderV6 can only read 0.6 files, found '$Attr{version}'\n" }
      } elsif($Name eq "osm"){
        $self->{state} = STATE_EXPECT_ENTITY;
        $self->{filetype} = FILETYPE_OSM;

        if( $Attr{version} ne "0.6" )
        { die "OsmChangeReaderV6 can only read 0.6 files, found '$Attr{version}'\n" }
      } else {
        die "Expected 'osmChange' tag, got '$Name'\n";
      }
    }
  }
  elsif( $self->{state} == STATE_EXPECT_COMMAND )
  {
    if($Name eq 'create' or $Name eq 'modify' or $Name eq 'delete'){
      $self->{command} = $Name;
      $self->{state} = STATE_EXPECT_ENTITY;
    } else {
      die "Expected command\n";
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
      if( $self->{filetype} == FILETYPE_OSMCHANGE )
      {
        $self->_process( $self->{command}, $self->{entity}, $self->{attr}, $self->{tags}, $self->{members} );
      }
      else  # FILETYPE_OSM
      {
        # Only entities with a modify tag are interesting, or if they have a negative ID (that's create)
        if( exists $self->{attr}->{action} and $self->{attr}->{action} eq "modify" and $self->{attr}{id} < 0 )
        { $self->{attr}->{action} = "create" }
        if( exists $self->{attr}->{action} )
        {
          $self->_process( $self->{attr}->{action}, $self->{entity}, $self->{attr}, $self->{tags}, $self->{members} );
        }
        elsif( $self->{attr}{id} < 0 )
        {
          $self->_process( "create", $self->{entity}, $self->{attr}, $self->{tags}, $self->{members} );
        }
      }
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
    if( $self->{filetype} == FILETYPE_OSMCHANGE )
    {
      if( $Name eq $self->{command} )
      {
        $self->{state} = STATE_EXPECT_COMMAND;
      }else {die}
    }
    else  # FILETYPE_OSM
    {
      if( $Name eq "osm" )
      {
        $self->{state} = STATE_INIT;
      } else {die}
    }
    return;
  }
  elsif( $self->{state} == STATE_EXPECT_COMMAND )
  {
    if( $Name eq "osmChange" )
    {
      $self->{state} = STATE_INIT;
    }else {die}
    return;
  }
}

1;

__END__

=head1 NAME

OsmChangeReaderV6 - Module for reading OpenStreetMap V6 Change XML data files

=head1 SYNOPSIS

  my $OSM = new Geo::OSM::OsmChangeReader(\&process);
  $OSM->load("Data/changes.osc");

  sub process
  {
    my($OSM, $command, $entity) = @_;
    print "Doing a $command on a $entity ".$entity->id."\n";
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
