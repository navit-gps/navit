package Geo::OSM::OsmXML;

use strict;
use warnings;

use Utils::File;
use Utils::Math;
use Utils::Debug;
use XML::Parser;

sub new(){ bless{} }

sub load(){
  my ($self, $file_name) = @_;

  my $start_time = time();
  my $P = new XML::Parser(Handlers => {Start => \&DoStart, End => \&DoEnd, Char => \&DoChar});
    my $fh = data_open($file_name);
    die "Cannot open OSM File $file_name\n" unless $fh;
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
sub name(){return($OsmXML::lastName)};

sub bounds(){
  my ($S,$W,$N,$E) = (1e+6,1e+6,-1e+6,-1e+6); # S,W,N,E
  foreach my $Node(values %OsmXML::Nodes){
    $S = $Node->{"lat"} if($Node->{"lat"} < $S);
    $N = $Node->{"lat"} if($Node->{"lat"} > $N);
    $W = $Node->{"lon"} if($Node->{"lon"} < $W);
    $E = $Node->{"lon"} if($Node->{"lon"} > $E);
  }
  return($S,$W,$N,$E);
}

# Function is called whenever an XML tag is started
sub DoStart()
{
  my ($Expat, $Name, %Attr) = @_;

  if($Name eq "node"){
    undef %OsmXML::Tags;
    %OsmXML::MainAttr = %Attr;
  }
  if($Name eq "segment"){
    undef %OsmXML::Tags;
    %OsmXML::MainAttr = %Attr;
  }
  if($Name eq "way"){
    undef %OsmXML::Tags;
    undef @OsmXML::WaySegments;
    %OsmXML::MainAttr = %Attr;
  }
  if($Name eq "tag"){
    $OsmXML::Tags{$Attr{"k"}} = $Attr{"v"};
  }
  if($Name eq "seg"){
    push(@OsmXML::WaySegments, $Attr{"id"});
  }
}

# Function is called whenever an XML tag is ended
sub DoEnd(){
  my ($Expat, $Element) = @_;
  if($Element eq "node"){
    my $ID = $OsmXML::MainAttr{"id"};
    $OsmXML::Nodes{$ID}{"lat"} = $OsmXML::MainAttr{"lat"};
    $OsmXML::Nodes{$ID}{"lon"} = $OsmXML::MainAttr{"lon"};
    foreach(keys(%OsmXML::Tags)){
      $OsmXML::Nodes{$ID}{$_} = $OsmXML::Tags{$_};
    }
  }
  if($Element eq "segment"){
    my $ID = $OsmXML::MainAttr{"id"};
    $OsmXML::Segments{$ID}{"from"} = $OsmXML::MainAttr{"from"};
    $OsmXML::Segments{$ID}{"to"} = $OsmXML::MainAttr{"to"};
    foreach(keys(%OsmXML::Tags)){
      $OsmXML::Segments{$ID}{$_} = $OsmXML::Tags{$_};
    }
  }
  if($Element eq "way"){
    my $ID = $OsmXML::MainAttr{"id"};
    $OsmXML::Ways{$ID}{"segments"} = join(",",@OsmXML::WaySegments);
    foreach(keys(%OsmXML::Tags)){
      $OsmXML::Ways{$ID}{$_} = $OsmXML::Tags{$_};
    }
  }
}

# Function is called whenever text is encountered in the XML file
sub DoChar(){
  my ($Expat, $String) = @_;
}

1;

__END__

=head1 NAME

OsmXML - Module for reading OpenStreetMap XML data files

=head1 SYNOPSIS

  $OSM = new OsmXML();
  $OSM->load("Data/nottingham.osm");

  foreach $Way(%OsmXML::Ways){
    @Segments = split(/,/, $Way->{"segments"});

    foreach $SegmentID(@Segments){
      $Segment = $OsmXML::Segments{$SegmentID};

      $Node1 = $OsmXML::Nodes{$Segment->{"from"}};
      $Node2 = $OsmXML::Nodes{$Segment->{"to"}};

      printf "Node at %f,%f, named %s, is a %s",
        $Node2->{"lat"},
        $Node2->{"lon"},
        $Node2->{"name"},
        $Node2->{"highway"};
      }
    }

=head1 AUTHOR

Oliver White (oliver.white@blibbleblobble.co.uk)

=head1 COPYRIGHT

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
