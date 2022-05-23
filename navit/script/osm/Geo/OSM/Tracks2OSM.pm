##################################################################
package Geo::OSM::Tracks2OSM;
# Functions:
# tracks2osm:
#     converts a tracks Hash to an OSM Datastructure
#
##################################################################

use Exporter;
@ISA = qw( Exporter );
use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);
@EXPORT = qw( tracks2osm );

use strict;
use warnings;

use Data::Dumper;

use Geo::Geometry;
use Geo::Tracks::Tools;
use Utils::Debug;
use Utils::File;
use Utils::Math;

my $first_id = -10000;

my $lat_lon2node={};
my $next_osm_node_number = $first_id;
my $osm_nodes_duplicate   = {};
# Check if a node at this position exists
# if it exists we get the old id; otherwise we create a new one
sub create_node($$) {
    my $osm_nodes = shift;
    my $elem      = shift;

    printf STDERR "create_node(): lat or lon undefined : $elem->{lat},$elem->{lon} ".Dumper(\$elem)."\n"
	unless  defined($elem->{lat}) && defined($elem->{lon}) ;

    my $id=0;
    my $lat_lon = sprintf("%f_%f",$elem->{lat},$elem->{lon});
    if ( defined( $osm_nodes_duplicate->{$lat_lon} ) ) {
	$id = $osm_nodes_duplicate->{$lat_lon};
	printf STDERR "Node already exists as $id	pos:$lat_lon\n"
	    if $DEBUG>2;
	$osm_nodes->{$id}=$elem;
	# TODO: We have to check that the tags of the old and new nodes don't differ
	# or we have to merge them
    } else {
	$next_osm_node_number--;
	$id = $next_osm_node_number;
	$elem->{tag}->{converted_by} = "Track2osm" ;
	$elem->{node_id} ||= $id;
	$osm_nodes->{$id}=$elem;
	$lat_lon2node->{$lat_lon}=$id;
	$osm_nodes_duplicate->{$lat_lon}=$id;
    };
    if ( !$id  ) {
	print STDERR "create_node(): Null node($id,$lat_lon) created\n".
	    Dumper($elem)
	    if $DEBUG;
    }
    return $id;
}


my $osm_way_number     = $first_id;
# ------------------------------------------------------------------
sub tracks2osm($){
    my $tracks = shift;


    my $osm_nodes = {};
    my $osm_ways  = {};
    my $reference = $tracks->{filename};

    my $last_angle         = 999999999;
    my $angle;
    my $angle_to_last;


    my $count_valid_points_for_ways=0;

    enrich_tracks($tracks);

    for my $track ( @{$tracks->{tracks}} ) {

	# We need at least two elements in track
	next unless scalar(@{$track})>1;

	$osm_way_number--;
	$osm_ways->{$osm_way_number}->{tag} =  {
	    "converted_by" => "Track2osm",
	    "highway"      => "FIXME",
	    "note"         => "FIXME",
	};
	for my $elem ( @{$track} ) {
	    my $node_id   = $elem->{node_id}      || create_node($osm_nodes,$elem);

	    # -------------------------------------------- Add to Way
	    push(@{$osm_ways->{$osm_way_number}->{nd}},$node_id);
	    $count_valid_points_for_ways++;
	}
    }

    my $bounds = GPS::get_bounding_box($tracks);
    printf STDERR "Track $reference Bounds: ".Dumper(\$bounds) if $DEBUG>5;
    return { nodes    => $osm_nodes,
	     ways     => $osm_ways,
	     bounds   => $bounds,
	 };
}



=head1 NAME

Geo::OSM::Track2OSM

=head1 COPYRIGHT

Copyright 2006, Jörg Ostertag

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

=head1 AUTHOR

Jörg Ostertag (planet-count-for-openstreetmap@ostertag.name)

=head1 SEE ALSO

http://www.openstreetmap.org/

=cut
