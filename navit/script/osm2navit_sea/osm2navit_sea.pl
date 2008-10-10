#!usr/bin/perl -w
#
# Vincent "latouche" Touchard, vtouchar@latouche.info
# 2008/09/10
#
# Transform a .osm file adding polygons representing the sea and islands.
# The sea is a way with the tag natural=coastline_outer and the islands are
# ways with the tag natural=coastline_inner.
# Adding those tags to osm2navit allow the see to be viewed by navit
#
# To avoid big polygons, the map is divided into small tiles (0.5°x0.5°)
#
# lisense: GPL
#
# history:
#  v1: first release
#  v2: 2008/09/06
#      - map split into tiles of 0.5x0.5°
#  v3: 2008/09/10
#      - detects islands vs lakes
#        takes each polygon and try to find is it is inside an other one. If true,
#        it is an island, if not, it is the sea. Should be a little more clever to detect
#        lake (tagged as natural=coastline instead of natural=water) inside islands.
#        This algo is not perfect but works in most cases
#      - sea tile with no way crossing are now drawn in a more clever way:
#        a node for each corner is added to nodes arrays and a way linking those
#        node is added. Those components are then added to the output file during 
#        the last section of this script. This allow to detects islands in such tiles
#        as islands and not sea
#
# TODO:
#  - better intersec_way_tiles (cf this fuction for more info)
#  - better lookup_handler (cf this fuction for more info)
#  - detect clockwise/anticlockwise ways => sea or land
#     for now -> every closed area is considered as land which is false.
#     some lakes are tags as natural=coastline instead of natural=water
#     ==> done, the algo should be improved for better results

use strict;
use warnings;

use Math::Trig;
use POSIX qw(ceil);
use way;

if ( @ARGV ne 6 ) {
	print "Usage: osm2navit_sea.pl minlat minlon maxlat maxlon source_file output_file\n";
	exit(1);
}


my $source=$ARGV[4];
my $destination=$ARGV[5];

my ($minlat, $minlong, $maxlat, $maxlong);
my ($minlat2, $minlong2, $maxlat2, $maxlong2);

$minlat=$ARGV[0];
$minlong=$ARGV[1];
$maxlat=$ARGV[2];
$maxlong=$ARGV[3];

$minlat2=int($minlat*2)/2;
$minlong2=int($minlong*2)/2;
$maxlat2=ceil($maxlat*2)/2;
$maxlong2=ceil($maxlong*2)/2;


#$rect_length=2*abs($maxlong-$minlong) + 2*abs($maxlat-$minlat);
#
#push (@corner_rect2lin, 0);
#push (@corner_rect2lin, abs($maxlong-$minlong));
#push (@corner_rect2lin, abs($maxlong-$minlong) + abs($maxlat-$minlat));
#push (@corner_rect2lin, $rect_length - abs($maxlat-$minlat));

my $osm_id=-1;

my (%new_ways, %nodes, %ways, %tiles, @new_nodes);

my ($i, $j, $way, $node, $tile);

my $debug=0;

sub debug {
	print shift if ( $debug );
}

# project a node on a bounding box
sub get_projection {
	my ($lat, $lon, $minlat, $minlon, $maxlat, $maxlon) = @_;

	my ($a, $b, $c, $d);

	$a = abs( $lat - $minlat);
	$b = abs( $lat - $maxlat);
	$c = abs( $lon - $minlon);
	$d = abs( $lon - $maxlon);


	if ( $a <= $b ) {
		if ( $a <= $c ) {
			if ( $a <= $d ) {
				# a
				$lat=$minlat;
				#$lon=$node->{"lon"};
			} else {
				# d
				#$lat=$node->{"lat"};
				$lon=$maxlon;
			}
		} else {
			if ( $c <= $d ) {
				# c
				#$lat=$node->{"lat"};
				$lon=$minlon;
			} else {
				#d
				#$lat=$node->{"lat"};
				$lon=$maxlon;
			}
		}
	} else {
		if ( $b <= $c ) {
			if ( $b <= $d ) {
				#b
				$lat=$maxlat;
				#$lon=$node->{"lon"};
			} else {
				# d
				#$lat=$node->{"lat"};
				$lon=$maxlon;
			}
		}  else {
			if ( $c <= $d ) {
				# c
				#$lat=$node->{"lat"};
				$lon=$minlon;
			} else {
				#d
				#$lat=$node->{"lat};
				$lon=$maxlon;
			}
		}
	}

	return (($lat, $lon));
}

# Projet a node on a rectangle to a segment
sub get_proj_rect2lin {
	my ($lat, $lon, $minlat, $minlon, $maxlat, $maxlon) = @_;


	if ($lat eq $maxlat) {
		return abs($minlon - $lon);
	} else {
		if ($lon eq $maxlon) {
			return abs($maxlon-$minlon) + abs($maxlat - $lat);
		} else {
			if ($lat eq $minlat) {
				return abs($maxlon-$minlon) + abs($maxlat-$minlat) + abs($maxlon-$lon);
			} else {
				return 2*abs($maxlon-$minlon) + abs($maxlat-$minlat) + abs($minlat - $lat);
			}
		}
	}
}

# projet a node from a segment to a rectangle
sub getnode_proj_lin2rect {
	my ($length, $minlat, $minlon, $maxlat, $maxlon) = @_;
	my ($lat, $lon);

	if ( $length > 2*abs($maxlon-$minlon) + abs($maxlat-$minlat)) {
		$lat = ($minlat + $length - 2*abs($maxlon-$minlon) - abs($maxlat-$minlat));
		$lon = ($minlon);
	} else {
		if ( $length > abs($maxlon-$minlon) + abs($maxlat-$minlat)) {
			$lat = ($minlat);
			$lon = ($maxlon - $length + abs($maxlon-$minlon) + abs($maxlat-$minlat));
		} else {
			if ( $length > abs($maxlon-$minlon)) {
				$lat = ($maxlat - $length + abs($maxlon-$minlon));
				$lon = ($maxlon);
			} else {
				$lat = ($maxlat);
				$lon = ($minlon + $length);
			}
		}
	}

	return (($lat, $lon));
}



# get the distance between to nodes on the same rectangle
sub get_path_length {
	my ($lat1, $lon1, $lat2, $lon2, @box) = @_;

	my ($a, $b, $rect_length, $dist);


	$a = get_proj_rect2lin ( $lat1, $lon1, @box);
	$b = get_proj_rect2lin ( $lat2, $lon2, @box);

	$dist = $b - $a;
	$rect_length = 2*abs( $box[2] - $box[0]) + 2*abs( $box[3] - $box[1]);
	return $dist > 0 ? $dist : $dist + $rect_length;
}

# get the nodes to go through to join an other node over a rectangle
sub get_path {
	my ($lat1, $lon1, $lat2, $lon2, @box) = @_;

	my ($a, $b, $rect_length, $dist, @corner_rect2lin);
	my @path=();

	$a = get_proj_rect2lin ( $lat1, $lon1, @box);
	$b = get_proj_rect2lin ( $lat2, $lon2, @box);

	$rect_length = 2*abs( $box[2] - $box[0]) + 2*abs( $box[3] - $box[1]);

	$dist = $b - $a;
	if ($dist <0) {
		$b+=$rect_length;
	}

	@corner_rect2lin = ( 0,
		abs( $box[3] - $box[1]),
		abs( $box[3] - $box[1]) + abs( $box[2] - $box[0]),
		$rect_length - abs( $box[2] - $box[0]));

	foreach (@corner_rect2lin) {
		if ($_ > $a && $_ < $b) {
			my ($lat, $lon) = getnode_proj_lin2rect ($_, @box);
			push (@path, {"lat" => $lat, "lon" => $lon});
		}
	}

	foreach (@corner_rect2lin) {
		if ($_ + $rect_length > $a && $_ + $rect_length < $b) {
			my ($lat, $lon) = getnode_proj_lin2rect ($_, @box);
			push (@path, {"lat" => $lat, "lon" => $lon});
		}
	}


	return @path;
}

# type of tile at zoom12 (land, sea, both, unknown.
# Uses oceantiles_12.dat for tiles@home project
# it checks the tile at level 12 containing the point ($lat, $lon)
# dx and dy are used to specify an offset: read tile (x+dx, y+dy)
# A more clever way whould be to check all the tile contained in and area
# and guess the final type
# one sea or more => sea
# one land or more => land
# only both or unknown => since tiles are quite big, should be land
#
sub lookup_handler {
	my ($lat, $lon, $dx, $dy) = @_;

	# http://wiki.openstreetmap.org/index.php/Slippy_map_tilenames
	# http://almien.co.uk/OSM/Tools/Coord
	my $zoom=12;
	# $tilex=int( ( $minlon +180)/360 *2**$zoom ) ;
	my $tilex=int( ( $lon  +180)/360 *2**$zoom ) ;
	# $tiley=int( (1 - log(tan( $maxlat  *pi/180) + sec( $maxlat  *pi/180))/pi)/2 *2**$zoom ) ;
	my $tiley=int( (1 - log(tan( $lat  *pi/180) + sec(  $lat  *pi/180))/pi)/2 *2**$zoom ) ;


	my $tileoffset = (($tiley+$dy) * (2**12)) + ($tilex+$dx);

	my $fh;
	open($fh, "<", "data/oceantiles_12.dat") or die ("Cannot open oceantiles_12.dat: $!");


	seek $fh, int($tileoffset / 4), 0;
	my $buffer;
	read $fh, $buffer, 1;
	$buffer = substr( $buffer."\0", 0, 1 );
	$buffer = unpack "B*", $buffer;
	my $str = substr( $buffer, 2*($tileoffset % 4), 2 );

	close($fh);

	#print "S" if $str eq "10";
	#print "L" if $str eq "01";
	#print "B" if $str eq "11";
	#print "?" if $str eq "00";

	return $str;
}


# return the intersection of a segment with the frontier
# between to tiles
# Doesn't handle the following cases:
# X    X
#  Y, Y , ...
#  Should return two nodes, one for the 1st tile, and an other one for
#  the second tile
sub intersec_way_tiles {
	my ($node1_lat, $node1_lon,
		$node2_lat,$node2_lon,
		$tile1_minlat, $tile1_minlon, $tile1_maxlat, $tile1_maxlon,
		$tile2_minlat, $tile2_minlon, $tile2_maxlat, $tile2_maxlon) = @_;
	my ($lat, $lon);

	print "intersec_way_tiles - $node1_lat, $node1_lon && $node2_lat,$node2_lon && $tile1_minlat, $tile1_minlon, $tile1_maxlat, $tile1_maxlon && $tile2_minlat, $tile2_minlon, $tile2_maxlat, $tile2_maxlon\n";

	if ( $tile1_minlat == $tile2_minlat ) { # tiles are side by side XY or YX
		$lon = ($tile1_minlon>$tile2_minlon) ? $tile1_minlon : $tile2_minlon;
		print "same lon: $lon\n";

		if ($node2_lon != $node1_lon) {
			$lat = $node1_lat + ($lon - $node1_lon) * ($node2_lat - $node1_lat) / ($node2_lon - $node1_lon); # thales
		} else {
			print "intersec_way_tiles - nodes with same lon... $node1_lat, $node1_lon & $node2_lat,$node2_lon\n";
			$lat = ($node2_lat + $node1_lat)/2;
		}
		print "lat: $lat\n";
	} else { # one tile is above the other
		$lat = ($tile1_minlat>$tile2_minlat) ? $tile1_minlat : $tile2_minlat;
		print "same lat: $lat\n";

		if ($node2_lon != $node1_lon) {
			$lon = $node1_lon + ($lat - $node1_lat) * ($node2_lon - $node1_lon) / ($node2_lat - $node1_lat);
		} else {
			print "intersec_way_tiles - nodes with same lat... $node1_lon, $node1_lon & $node2_lat,$node2_lon\n";
			$lon = ($node2_lon + $node1_lon)/2;
		}
		print "lon: $lon\n";
	}

	return ($lat, $lon);
}

# tells if a node is inside a polygon or not
# not efficient a 100%, but quite good results
# widely inspired from close-areas.pl from the T@h project
sub polygon_contains_node {
	my ($way, $node) = @_;


	my $counter = 0;

	my @way_nodes = $way->nodes();
	my $p1 =  $nodes{$way->first_node()};
	my $n = scalar(@way_nodes);

	for (my $i=1; $i < $n; $i++) {  
		my $p2 = $nodes{$way_nodes[$i]};

		if ($node->{"lat"} > $p1->{"lat"} || $node->{"lat"} > $p2->{"lat"}) {  
			if ($node->{"lat"} < $p1->{"lat"} || $node->{"lat"} < $p2->{"lat"}) {  
				if ($node->{"lon"} < $p1->{"lon"} || $node->{"lon"} < $p2->{"lon"}) {  
					if ($p1->{"lat"} != $p2->{"lat"}) {  
						my $xint = ($node->{"lat"}-$p1->{"lat"})*($p2->{"lon"}-$p1->{"lon"})/($p2->{"lat"}-$p1->{"lat"})+$p1->{"lon"};
						if ($p1->{"lon"} == $p2->{"lon"} || $node->{"lon"} <= $xint) {  
							$counter++;
						}                                                                                                                  
					}                                                                                                                      
				}
			}
		}
		$p1 = $p2;
	}

	return ($counter%2);
}

#################################
print "initialising tiles\n";


debug("there will be ".ceil(abs($maxlong2-$minlong2)/0.5)."x".ceil(abs($maxlat2-$minlat2)/0.5)." tiles\n");

my $nb_tiles_x = ceil(abs($maxlat2-$minlat2)/0.5);
my $nb_tiles_y = ceil(abs($maxlong2-$minlong2)/0.5);

for ($j=0; $j<$nb_tiles_x; $j++) {
	for ($i=0; $i<$nb_tiles_y; $i++) {


		my $type = lookup_handler ( $maxlat2-0.5*$j, $minlong2+0.5*$i, 0, 0);

		print "S" if $type eq "10";
		print "L" if $type eq "01";
		print "B" if $type eq "11";
		print "?" if $type eq "00";
		$tiles{$nb_tiles_x * $i+$j} = { "lon" => $minlong2+0.5*$i, "lat" => $maxlat2-0.5*$j, "type" => $type, "id" => ($nb_tiles_x * $i+$j), "crossed" => 0};
	}
	print "\n";
}

print "\n";



################################
# find coastlines and store the nodes id
print  "Reading osm source file\n";

my $input;
my $output;
open($input, "<$source") or die "Can't open $source: $!";

my $way_id=0;
my $is_coastline=0;
my @nodes;
while (<$input>) {

	if ( /<way id=["'](\d+)["']/) {
		$way_id=$1;
		$is_coastline=0;
		@nodes=();
	}
	if ($way_id != 0) {
		if ( /<nd ref=["'](\d+)["']/) {
			push(@nodes, $1);
		}
		if ( /<tag k=["']natural["'] v=["']coastline["']/) {
			$is_coastline=1;
		}
		if ( /<\/way/ ) {
			# forget node with only one element -> disturb the algo
			if ( $is_coastline && scalar(@nodes) != 1 ) {
				my $way=way->new($way_id);
				$way->nodes(@nodes);
				debug( "found coastline ".$way->id()." from ".$way->first_node()." to ".$way->last_node()." with ".$way->size()." nodes\n");
				$ways{$way->id()}=$way;
			}

			$way_id=0;
		}
	}
}

close($input);




###########################
# to save memory, only retrieve usefull nodes
print "Retrieving nodes:\n";

$i=0;
foreach (values %ways) {
	foreach ($_->nodes()) {
		$nodes{$_}={};
		$i++;
	}
}
debug("Created $i nodes\n");

open($input, "<$source") or die "Can't open $source: $!";
open($output, ">$destination") or die "Can't open $destination: $!";

$i=0;
while (<$input>) {
	if ( /<node id=['"](\d+)['"].* lat=['"](.+)['"] lon=['"](.+)['"]/ ) {
		if ( exists($nodes{$1}) ) {
			$nodes{$1}->{"id"}=$1;
			$nodes{$1}->{"lat"}=$2;
			$nodes{$1}->{"lon"}=$3;
			$i++;
		}
	}
	print $output $_ unless ( /<\/osm>/ );
}
close($input);

debug("Retrieved $i nodes\n");



############################
# If the bounding box given as arguments to the program is smaller than the one used to generate
# the input file, some nodes may be outside -> check for this before adding them to a tile
print "Spliting data into tiles:\n";


foreach $way (values %ways) {
	#print "following new way\n";

	my $new_way = way->new($osm_id--);
	my @way_nodes = ();
	my ($tile_id, $new_tile_id);
	$tile_id = -1;
	my $prev_node;

	foreach $node ($way->nodes()) {
		my $tile_lat = int( ($maxlat2 - $nodes{$node}->{"lat"} ) / 0.5);
		my $tile_lon = int( ($nodes{$node}->{"lon"} - $minlong2) / 0.5);
		$new_tile_id = $nb_tiles_x * $tile_lon + $tile_lat;

		if ( $tile_id == -1 ) {
			$tile_id = $new_tile_id;
		}
		# we have reach an other tile
		if ( $new_tile_id != $tile_id ) {

				my $tile = $tiles{$tile_id};
				my $new_tile = $tiles{$new_tile_id};

				# find the intersection of the way with the tiles
				my ($lat, $lon) = intersec_way_tiles (
					$nodes{$node}->{"lat"}, $nodes{$node}->{"lon"},
					$nodes{$prev_node}->{"lat"}, $nodes{$prev_node}->{"lon"},
					$tile->{"lat"}-0.5, $tile->{"lon"}, $tile->{"lat"}, $tile->{"lon"}+0.5,
					$new_tile->{"lat"}-0.5, $new_tile->{"lon"}, $new_tile->{"lat"}, $new_tile->{"lon"}+0.5);
				my $mid_node = {"lat" => $lat, "lon" => $lon, "id" => $osm_id-- };

				$nodes{ $mid_node->{"id"} } = $mid_node;
				push( @new_nodes, $mid_node );

				push ( @way_nodes, $mid_node->{"id"} );
			
				$new_way->nodes(@way_nodes);
				$new_ways{$new_way->id()} = $new_way;	
				$ways{$new_way->id()} = $new_way;
				push( @{$tiles{$tile_id}->{"ways"}}, $new_way->id());

			# set both new and old tile as crossed by a way
			# the new one is important because when we rich the end
			# of the coast, there will be no "next way" to set it as
			# crossed
			$tiles{$tile_id}->{"crossed"}=1;
			$tiles{$new_tile_id}->{"crossed"}=1;

			$new_way = way->new($osm_id--);

			@way_nodes=();
			push ( @way_nodes, $mid_node->{"id"} );

			$tile_id = $new_tile_id;
		}

		push (@way_nodes, $node);
		#print "Node $node added to tile $tile_id\n";
		$prev_node=$node;
	}
	if ( scalar(@way_nodes) > 1) {
		$new_way->nodes(@way_nodes);
		$new_ways{$new_way->id()} = $new_way;	
		$ways{$new_way->id()} = $new_way;
		push( @{$tiles{$tile_id}->{"ways"}}, $new_way->id());
	}

	$way->distroy();

}







##############################################
print "\n\nWorking on tiles:\n";


#$tile = $tiles{54};
foreach $tile (values %tiles) {

	debug( "->tile: ".$tile->{"id"}."\n");

	my @box = ($tile->{"lat"}-0.5, $tile->{"lon"}, $tile->{"lat"}, $tile->{"lon"}+0.5);

	# No way going through the tile -> guess if it is sea or lanf
	# SHould check if it containds nodes -> type unknown + nodes should mean sea + islands
	unless ($tile->{"crossed"}) {
		my $tile_is_sea=0;

		#  00 => unknown
		#  01 => land
		#  10 => sea
		#  11 => coast

		# water
		if ($tile->{"type"} eq "10") {
			$tile_is_sea=1;

		} elsif ($tile->{"type"} ne "01") { # not land

			my %temp = ("00"=>0, "10"=>0, "01"=>0, "11"=>0);
			$temp{lookup_handler($tile->{"lat"}, $tile->{"lon"}, -1, 0)}++;
			$temp{lookup_handler($tile->{"lat"}, $tile->{"lon"}, +1, 0)}++;
			$temp{lookup_handler($tile->{"lat"}, $tile->{"lon"}, 0, -1)}++;
			$temp{lookup_handler($tile->{"lat"}, $tile->{"lon"}, 0, +1)}++;

			if( $temp{"10"} > $temp{"01"} ) {
				$tile_is_sea=1;
			} elsif ( ($tile->{"type"} eq "11") and ( $temp{"01"} == 0 ) ) {
				# if the tile is marked coast but no land near, assume it's a group of islands instead of lakes.
				$tile_is_sea=1;
			} # else = land

		}

		# add nodes and a way to draw the sea all over the tile
		if ($tile_is_sea) {
			my @way_nodes_id = ( $osm_id, $osm_id-1, $osm_id-2, $osm_id-3);
			my @way_nodes = (
				{ "lat" => $tile->{"lat"}, "lon" => $tile->{"lon"}, "id" => $osm_id--},
				{ "lat" => $tile->{"lat"}, "lon" => $tile->{"lon"}+0.5, "id" => $osm_id--},
				{ "lat" => $tile->{"lat"}-0.5, "lon" => $tile->{"lon"}+0.5, "id" => $osm_id--},
				{ "lat" => $tile->{"lat"}-0.5, "lon" => $tile->{"lon"}, "id" => $osm_id--});

			my $way = way->new($osm_id--);
			$way->nodes(@way_nodes_id);

			push( @new_nodes, @way_nodes );
			foreach (@way_nodes) {
				$nodes{$_->{"id"}} = $_;
			}

			$new_ways{$way->id()} = $way;	
			$ways{$way->id()} = $way;
			push( @{$tile->{"ways"}}, $way->id());
		}
	}

	# next tile if there is nothing in this one
	next if ( ! $tile->{"ways"} );

	#========================================
	# merge ways that can be merged
	print "\tBuilding tree:\n";

	foreach (@{$tile->{"ways"}}) {	
		$way = $new_ways{$_};
		debug( "node ".$way->id()." (size: ".$way->size().")\n" );
		# only for not yet complete polygons and ways without prev and next
		if ($way->next() == 0 || $way->prev() == 0) {
			foreach (@{$tile->{"ways"}}) {
				my $way2=$new_ways{$_};
				if ($way->last_node() == $way2->first_node() ) {
					if ($way->next() == 0) {
						debug( "\tnext = ".$way2->id()."\n");
						$way->next($way2->id());
					} else {
						debug( "\t error: next way duplicated: old:".$way->next()." new:".$way2->id()." - node: ".$way->last_node()."\n");
					}
				}
				if ($way->first_node() == $way2->last_node() ) {
					if ($way->prev() == 0) {
						debug( "\tprev = ".$way2->id()."\n");
						$way->prev($way2->id());
					} else {
						debug( "\t error: prev way duplicated: old:".$way->prev()." new:".$way2->id()." - node: ".$way->first_node()."\n");
					}
				}
			}
		} else {
			debug( "\tAlready ok\n" );
		} 
	}
	#=============================
	# if some ways are not closed, we need to close them.
	# It is done by adding new nodes on the border of the tile
	# and joining them
	print ("\n\nClosing loops:\n");

	my (@first_nodes, @last_nodes);

	# project nodes on the border of the tile
	# Some nodes are already on the border of the tile (mainly because of 
	# intersec_way_tiles)
	foreach (@{$tile->{"ways"}}) {	
		my $way = $new_ways{$_};

		next if ( $way->first_node() == $way->last_node()); # already a loop

		if ( $way->prev() == 0) {
			my @way_nodes = $way->nodes();

			my $lat = $nodes{$way_nodes[0]}->{"lat"};
			my $lon = $nodes{$way_nodes[0]}->{"lon"};
			($lat, $lon) = get_projection( $lat, $lon, $tile->{"lat"}-0.5, $tile->{"lon"}, $tile->{"lat"}, $tile->{"lon"}+0.5 );
			my $node = { "lat" => $lat, "lon" => $lon, "id" => ($osm_id--)};
			unshift( @way_nodes, $node->{"id"} );
			push( @new_nodes, $node );
			$nodes{$node->{"id"}}=$node;
			push( @first_nodes, $node->{"id"});

			$way->nodes(@way_nodes);
		}
		if ( $way->next() == 0) {

			my @way_nodes = $way->nodes();

			my $lat = $nodes{$way_nodes[ @way_nodes - 1 ]}->{"lat"};
			my $lon = $nodes{$way_nodes[ @way_nodes - 1 ]}->{"lon"};
			($lat, $lon) = get_projection( $lat, $lon, $tile->{"lat"}-0.5, $tile->{"lon"}, $tile->{"lat"}, $tile->{"lon"}+0.5 );
			my $node = { "lat" => $lat, "lon" => $lon, "id" => $osm_id--};
			push( @way_nodes, $node->{"id"} );
			push( @new_nodes, $node );
			$nodes{$node->{"id"}}=$node;
			push( @last_nodes, $node->{"id"});

			$way->nodes(@way_nodes);

		}

	}

	# link nodes -> find a way around the tile


	my $last;
	foreach $last (@last_nodes) {
		print "Looking for path ...\n";
		my ($path1, $path2);
		$path1=0;

		# find the sortest path between nodes
		# -> will give the next node around the tile
		my $first;
		my $first_elected;
		foreach $first (@first_nodes) {
			$path2=get_path_length($nodes{$last}->{"lat"},
				$nodes{$last}->{"lon"},
				$nodes{$first}->{"lat"},
				$nodes{$first}->{"lon"},
				@box);
			if ($path1 == 0 || $path1 > $path2) {
				$path1=$path2;
				$first_elected=$first;
			}
		}

		# get the path around the tile between the two nodes
		my @path = get_path ( $nodes{$last}->{"lat"},
			$nodes{$last}->{"lon"},
			$nodes{$first_elected}->{"lat"},
			$nodes{$first_elected}->{"lon"},
			@box);

		foreach (@path) {
			$_->{"id"} = $osm_id--;
			push(@new_nodes, $_);
			$nodes{$_->{"id"}} = $_;
		}


		unshift(@path, $nodes{$last});
		push(@path, $nodes{$first_elected});

		my $way = way->new($osm_id--);
		my @way_nodes=();
		debug( "new way ".$way->id().":\n");
		foreach (@path) {
			push(@way_nodes, $_->{"id"});
			debug( "nodes id: ".$_->{"id"}."\tlat: ".$_->{"lat"}." - long: ".$_->{"lon"}."\n");
		}
		$way->nodes(@way_nodes);

		push( @{$tiles{$tile->{"id"}}->{"ways"}}, $way->id());
		$new_ways{$way->id()} = $way;
		$ways{$way->id()} = $way;



	}


	# linking the ways
	# This time all ways should be loops because of the ways we added before
	print "\n\nBuilding tree with new ways:\n";

	foreach (@{$tile->{"ways"}}) {	
		my $way = $new_ways{$_};
		debug( "node ".$way->id()." (size: ".$way->size().")\n" );
		# only for not yet complete polygons and ways without prev and next
		if ($way->next() == 0 || $way->prev() == 0) {
			foreach (@{$tile->{"ways"}}) {
				my $way2=$new_ways{$_};
				if ($way->last_node() == $way2->first_node() ) {
					if ($way->next() == 0) {
						debug( "\tnext = ".$way2->id()."\n");
						$way->next($way2->id());
					} else {
						debug( "\t error: next way duplicated: old:".$way->next()." new:".$way2->id()." - node: ".$way->last_node()."\n");
					}
				}
				if ($way->first_node() == $way2->last_node() ) {
					if ($way->prev() == 0) {
						debug( "\tprev = ".$way2->id()."\n");
						$way->prev($way2->id());
					} else {
						debug( "\t error: prev way duplicated: old:".$way->prev()." new:".$way2->id()." - node: ".$way->first_node()."\n");
					}
				}
			}
		} else {
			debug( "\tAlready ok\n" );
		} 
	}


	#merge ways to have only simple ways that are circular
	print "merge ways\n";

	# copy into @a because we add new ways to the array inside the loop
	# -> otherwise it creates an infinite loop
	my @a = @{$tile->{"ways"}};
	foreach (@a) {
#		print "way $_\n";
		my $way = $new_ways{$_};
		next unless ($way->is_alive());

		my @way_nodes;	
		while ($way->is_alive()) {

			push (@way_nodes, $way->nodes());

			# Distroy the way -> it won't be counted twince thus this will stop the loop
			# and it will free some memory
			$way->distroy();
			last if ($way->next() == 0);
			$way=$new_ways{ $way->next() };
		}
		my $new_way = way->new($osm_id--);
		$new_way->nodes(@way_nodes);

		push( @{$tiles{$tile->{"id"}}->{"ways"}}, $new_way->id());
		$new_ways{$new_way->id()} = $new_way;
		$ways{$new_way->id()} = $new_way;

	}


	## sort the ways between sea and land
	foreach (@{$tile->{"ways"}}) {	
		my $way = $new_ways{$_};
		next unless ($way->is_alive());
#				print "---- ".$way->id().": ".$way->first_node()." -> ".$way->last_node()."\n";
		foreach (@{$tile->{"ways"}}) {	
			my $way2 = $new_ways{$_};
			# - don't check if a way is included in itself
			# - check only for ways of type "outer" which is the default type
			#    => actualy should ckeck it. if a not is inside a "inner" way then it belongs
			#    to a lake that is in an island
			if ($way2->is_alive() && $way2->id() != $way->id() && $way2->type() eq "outer") {
				if (polygon_contains_node($way2, $nodes{$way->first_node()})) {
					$way->type("inner");
					last;
				}
			}
		}
	}
}












########################################
print "\n\nGenerating new data:\n";

# add new nodes to output file
foreach $node (@new_nodes) {
	debug( "adding node ".$node->{"id"}." to osm file\n");
	print $output "  <node id=\"".$node->{"id"}."\" timestamp=\"2008-07-02T22:57:53Z\" user=\"latouche\" lat=\"".$node->{"lat"}."\" lon=\"".$node->{"lon"}."\"/>\n";
}

# add new ways (highway=motorway for debug)
#foreach $way (values %new_ways) {
#	print $output "  <way id=\"".$way->id()."\" timestamp=\"2008-07-02T22:57:53Z\" user=\"latouche\">\n";
#	foreach $node ($way->nodes()) {
#		print $output "    <nd ref=\"".$node."\"/>\n" if defined $node;
#	}
#
#	print $output "    <tag k=\"highway\" v=\"motorway\"/>\n";
#	print $output "  </way>\n";
#
#}


# add new ways to output file
foreach $tile (values %tiles) {
	debug( "Writing tile: ".$tile->{"id"}."\n");
	foreach (@{$tile->{"ways"}}) {
		my $way = $new_ways{$_};
		my $type = "inner";
		next unless ($way->is_alive());
		print $output "  <way id=\"".($osm_id--)."\" timestamp=\"2008-07-02T22:57:53Z\" user=\"latouche\">\n";
		while ($way->is_alive()) {

			$type = ($way->type() eq "outer") ? "water" : "land";

			foreach $node ($way->nodes()) {
				print "undef node in ".$way->id()."\n" unless defined $node;
				print $output "    <nd ref=\"".$node."\"/>\n" if defined $node;
			}
			$way->distroy();
			last if ($way->next() == 0);
			$way=$new_ways{ $way->next() };
		}
		print $output "    <tag k=\"natural\" v=\"".$type."\"/>\n";
		print $output "  </way>\n";
	}
}


print $output "</osm>\n";

close($output);

exit(0);
