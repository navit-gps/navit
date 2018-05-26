##################################################################
package Geo::OSM::SegmentList;
##################################################################

use Exporter;
@ISA = qw( Exporter );
use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);
@EXPORT = qw( LoadOSM_segment_csv
	      reduce_segments_list
	      read_osm_file
	      load_segment_list
	      );

use strict;
use warnings;

use Math::Trig;

use Data::Dumper;
use Geo::Geometry;
use Geo::OSM::Planet;
use Utils::Debug;
use Utils::File;
use Utils::Math;
use Geo::Filter::Area;

sub load_segment_list($;$){
    my $do_filter_against_osm = shift;
    my $bounds  = shift;

    my $osm_segments;
    print STDERR "load_segment_list from: '$do_filter_against_osm'\n";
    if (  $do_filter_against_osm =~ m/^postgis|mapnik$/ ) {
	$osm_segments = Geo::OSM::SegmentList::LoadOSM_segment_postgis($bounds);
    } elsif ( -s $do_filter_against_osm ) {
	if (  $do_filter_against_osm =~ m/\.csv/ ) {
	    $osm_segments = Geo::OSM::SegmentList::LoadOSM_segment_csv($do_filter_against_osm, $bounds);
	} elsif ( $do_filter_against_osm =~ m/\.osm/ ) {
	    $osm_segments = Geo::OSM::SegmentList::read_osm_file($do_filter_against_osm, $bounds);
	} else {
	    die "Unknown Datatype for $do_filter_against_osm\n";
	}
	#print Dumper(\$osm_segments ) if $DEBUG;
    } elsif (  $do_filter_against_osm !~ m/^\d*$/ ) {
	    die "Unknown Datatype for $do_filter_against_osm\n";
    } else {

	# later we search in:
	#  ~/.osm/data/planet.osm.csv
	# /var/data/osm/planet.osm.csv

	my $home = $ENV{HOME}|| '~/';
	my $path=	planet_dir();
	my $osm_filename = "${path}/csv/osm-segments.csv";
	$osm_filename =~ s,\~/,$home/,;
	printf STDERR "check $osm_filename for loading\n" if $DEBUG;

	die "Cannot open $osm_filename\n" unless -s $osm_filename;
	$osm_segments = Geo::OSM::SegmentList::LoadOSM_segment_csv($osm_filename, $bounds);
    };
    return $osm_segments
}

# ------------------------------------------------------------------
# reduce osm Segments to only those inside the bounding box
# This make comparison faster
sub reduce_segments_list($$) {
    my $all_osm_segments = shift;
    my $bounds           = shift;

    my $start_time=time();

    #printf STDERR "reduce_osm_segments(".Dumper(\$bounds).")\n" if $DEBUG;

    my $osm_segments = [];
    my $count=0;
    my $all_count=0;
    for my $segment ( @{$all_osm_segments} ) {
	$all_count++;
	next unless $segment->[0] >= $bounds->{lat_min};
	next unless $segment->[0] <= $bounds->{lat_max};
	next unless $segment->[1] >= $bounds->{lon_min};
	next unless $segment->[1] <= $bounds->{lon_max};
	next unless $segment->[2] >= $bounds->{lat_min};
	next unless $segment->[2] <= $bounds->{lat_max};
	next unless $segment->[3] >= $bounds->{lon_min};
	next unless $segment->[3] <= $bounds->{lon_max};
	$count++;
	push(@{$osm_segments},$segment);
    }
    if ( $VERBOSE > 3 || $DEBUG > 3 ) {
	printf STDERR "		Reduced OSM Segment list to $count( $all_count) OSM-Segments ";
	print_time($start_time);
    }

    return $osm_segments;
}

# -------------------------------------------------------
# Load the csv Version of a segment list
# Args: $filename, {lat_min=> .., lat_max => ..., lon_min => .., lon_max => .. }
sub LoadOSM_segment_csv($;$){
    my $filename = shift;
    my $bounds  = shift;
    printf STDERR "Reading OSM-Segment-csv File: $filename, ($bounds->{lat_min} ... $bounds->{lat_max} , $bounds->{lon_min} ... $bounds->{lon_max})\n"
	if $DEBUG || $VERBOSE;
    my $start_time=time();

    my $segments;
    my $check_bounds = 1 if defined $bounds;
    $main::dont_write_osm_storable=1 ||  $check_bounds;
    $main::dont_read_osm_storable=1;

    if ( -s "$filename.storable" &&
	 ! file_needs_re_generation($filename,"$filename.storable")
	 && ! $main::dont_read_osm_storable ) {
	# later we should compare if the file also is newer than the source
	$filename .= ".storable";
	printf STDERR "Reading OSM File as storable: $filename\n"
	    if $DEBUG || $VERBOSE;
	$segments = Storable::retrieve($filename);
	if ( $VERBOSE >1 || $DEBUG) {
	    printf STDERR "Reading $filename done";
	    print_time($start_time);
	}
    } else {
	my $fh = data_open($filename);
	my $count=0;
	my $count_read=0;

	die "Cannot open $filename in LoadOSM_segment_csv.\n".
	    "Please create it first to use the option --osm.\n".
	    "See --help for more info"  unless $fh;

	while ( my $line = $fh ->getline() ) {
	    $count++;
	    chomp $line;
	    my @segment;
	    my $dummy;
	    ($segment[0],$segment[1],$segment[2],$segment[3],$segment[4]) = split(/,/,$line,5);
	    #print STDERR Dumper(\@segment);

	    if ( $check_bounds ) {
		next unless $segment[0] >= $bounds->{lat_min};
		next unless $segment[0] <= $bounds->{lat_max};
		next unless $segment[1] >= $bounds->{lon_min};
		next unless $segment[1] <= $bounds->{lon_max};
		next unless $segment[2] >= $bounds->{lat_min};
		next unless $segment[2] <= $bounds->{lat_max};
		next unless $segment[3] >= $bounds->{lon_min};
		next unless $segment[3] <= $bounds->{lon_max};
	    }

	    push (@{$segments},\@segment);
	    $count_read++;
	}
	$fh->close();
	if ( $VERBOSE >1 || $DEBUG) {
	    printf STDERR "Read and parsed $count_read($count) Lines in $filename";
	    print_time($start_time);
	}
	if ( ! $main::dont_write_osm_storable ) {
	    $start_time=time();
	    Storable::store($segments   ,"$filename.storable");
	    if ( $VERBOSE >1 || $DEBUG) {
		printf STDERR "Wrote Storable in to $filename.storable";
		print_time($start_time);
	    }
	};
    }


    return($segments);
}



# -------------------------------------------------------
# Load the segment list from postgis
# Args: {lat_min=> .., lat_max => ..., lon_min => .., lon_max => .. }
# This is a test to see if we can handle all segments of the world
# with reasonable query times
sub LoadOSM_segment_postgis($;$){
    my $bounds  = shift;
    printf STDERR "Reading OSM-Segments form PostGis($bounds->{lat_min} ... $bounds->{lat_max} , $bounds->{lon_min} ... $bounds->{lon_max})\n"
	if $DEBUG || $VERBOSE;
    my $start_time=time();

    my $segments;
    my $check_bounds = 1 if defined $bounds;
    my @row;
    my $sth;

    use DBI;

    my $dbname="gis";
    my $dbhost="/var/run/postgresql";
    my $dbh = DBI->connect("dbi:Pg:dbname=$dbname;host=$dbhost", "", "", {AutoCommit => 0});

    unless( $dbh) {
	warn "\n\n!!!!!!!!!!!!!!!! WARNING: Cannot Open Database for reading OSM Segments\n\n";
	return;
    };

    # WHERE  the_geom && setsrid('BOX(47.36 21.40,51.185 21.53)'::box2d, 42102)
    # WHERE GeomFromText('POINT(25.3 25.40)', 42102) && the_geom \
    #       AND distance(the_geom, GeomFromText('POINT(25.7 5.3)', 42102)) = 0
    # WHERE distance(the_geom, setsrid('BOX(4.36 2.3,1.5 2.8)'::box2d, 42102)) = 0
    # WHERE  the_point && setsrid('BOX(295149 2315499, 465992 2163790)'::box2d, -1)

    ##############################
    # FOR DEBUG
#    $bounds->{lat_min}=10;
#    $bounds->{lon_min}=10;
#    $bounds->{lat_max}=70;
#    $bounds->{lon_max}=70;
    #
    ####################################

    my $SRID_WGS84="4326";
    my $SRID_UNKNOWN="3395";
    my $SRID_OSM="900913";

    my $polygon_text=sprintf("POLYGON((%.5f %.5f,%.5f %.5f,%.5f %.5f,%.5f %.5f,%.5f %.5f))",
			     $bounds->{lat_min}-0.1 , $bounds->{lon_min}-0.1,
			     $bounds->{lat_max}+0.1 , $bounds->{lon_min}-0.1 ,
			     $bounds->{lat_max}+0.1 , $bounds->{lon_max}+0.1 ,
			     $bounds->{lat_min}-0.1 , $bounds->{lon_max}+0.1 ,
			     $bounds->{lat_min}-0.1 , $bounds->{lon_min}-0.1);
    my $polygon="transform(GeomFromText('$polygon_text',${SRID_WGS84}),$SRID_OSM)";

    my $select = "select osm_id,highway,name from planet_osm_line where highway is not null and way && ".
	" $polygon ".
	"limit 5;";
    print "\nprepare: $select\n";
    $sth = $dbh->prepare($select, { pg_server_prepare => 1 });
    $sth->execute();
    while ( @row = $sth->fetchrow_array ) {
	print "postgis ROW: @row\n";
    }
    printf STDERR "---------------------------------------------------------------\n" if $DEBUG || $VERBOSE;


#     $select = "SELECT name ,astext(way),$distance FROM planet_osm_roads WHERE  $distance < 500 order by  $distance;";

    #SELECT * FROM planet_osm_roads WHERE way in ?",
    #$select = "SELECT name FROM planet_osm_roads WHERE distance(way,GeomFromText('POINT(856371.58 6683083.41)', 3395)) < 1000;";
#    my $distance="distance(way,     GeomFromText('POINT(856371.58 6683083.41)', 3395))";
    my $point_text=sprintf("POINT(%.5f %.5f)",     $bounds->{lat_min} , $bounds->{lon_min});
    my $point="transform(GeomFromText('$point_text',${SRID_WGS84}),$SRID_OSM)";
    my $distance="distance(way,$point)";

    $select = "SELECT name FROM planet_osm_line WHERE ${distance} < 1000;";
    print "\nprepare: $select\n";
    $sth = $dbh->prepare($select, { pg_server_prepare => 1 });
    $sth->execute();
    while ( @row = $sth->fetchrow_array ) {
	print "postgis ROW: @row\n";
    }
    printf STDERR "---------------------------------------------------------------\n" if $DEBUG || $VERBOSE;

    $select = "SELECT name FROM planet_osm_roads WHERE ${distance} < 1000;";
    print "\nprepare: $select\n";
    $sth = $dbh->prepare($select, { pg_server_prepare => 1 });
    $sth->execute();
    while ( @row = $sth->fetchrow_array ) {
	print "postgis ROW: @row\n";
    }
    printf STDERR "---------------------------------------------------------------\n" if $DEBUG || $VERBOSE;

    $select = "SELECT osm_id,highway,name,asText(transform(way,${SRID_WGS84})) from planet_osm_line where highway is not null  limit 5;";
    print "\nprepare: $select\n";
    $sth = $dbh->prepare($select, { pg_server_prepare => 1 });
    $sth->execute();
    while ( @row = $sth->fetchrow_array ) {
	print "postgis ROW: @row\n";
    }

    printf STDERR "---------------------------------------------------------------\n" if $DEBUG || $VERBOSE;

    my $count=0;
    my $count_read=0;
    while ( @row = $sth->fetchrow_array ) {
	print "postgis ROW: @row\n";

	$count++;
	my $line="";
	chomp $line;
	my @segment;
	my $dummy;
	($segment[0],$segment[1],$segment[2],$segment[3],$segment[4]) = split(/,/,$line,5);
	#print STDERR Dumper(\@segment);

	if ( $check_bounds ) {
	    next unless $segment[0] >= $bounds->{lat_min};
	    next unless $segment[0] <= $bounds->{lat_max};
	    next unless $segment[1] >= $bounds->{lon_min};
	    next unless $segment[1] <= $bounds->{lon_max};
	    next unless $segment[2] >= $bounds->{lat_min};
	    next unless $segment[2] <= $bounds->{lat_max};
	    next unless $segment[3] >= $bounds->{lon_min};
	    next unless $segment[3] <= $bounds->{lon_max};
	}

	push (@{$segments},\@segment);
	$count_read++;
    }

    $dbh->disconnect;


    return($segments);
}


# ----------------------
sub Storable_save($$){
    my $filename = shift;
    my $segments = shift;
    eval{
	Storable::store($segments   ,"$filename.storable");
	};
    if ( $@ ) {
	#warn Dumper(\$segments);
	die "Storable_save(): $@\n";
    }
    printf STDERR "Stored OSM File: $filename as storable\n"
	if $DEBUG || $VERBOSE;
}

# ----------------------
sub Storable_load($){
    my $filename = shift;
    $filename .= ".storable";
    my $segments  = Storable::retrieve($filename);
    printf STDERR "Loaded OSM File: $filename as storable\n"
	if $DEBUG || $VERBOSE;
    return $segments;
}

##################################################################
# read Segment list from osm File
##################################################################

#our $read_osm_nodes;
our $read_osm_segments;
#our $read_osm_obj;
our (%MainAttr,$Type,%Tags, @WaySegments);
# Stats
our %AllTags;
# Stored data
our (%Nodes, %Segments, %Stats);
our $AREA_FILTER;
$AREA_FILTER = Geo::Filter::Area->new( area => "world" );
my $from_node=0;

# Function is called whenever an XML tag is started
#----------------------------------------------
sub DoStart()
{
    my ($Expat, $Name, %Attr) = @_;

    if($Name eq "node"){
	undef %Tags;
	%MainAttr = %Attr;
	$Type = "n";
    }
    if($Name eq "segment"){
	undef %Tags;
	%MainAttr = %Attr;
	$Type = "s";
    }
    if($Name eq "way"){
	undef %Tags;
	undef @WaySegments;
	%MainAttr = %Attr;
	$Type = "w";
    }
    if($Name eq "tag"){
	# TODO: protect against id,from,to,lat,long,etc. being used as tags
	$Tags{$Attr{"k"}} = $Attr{"v"};
	$AllTags{$Attr{"k"}}++;
	$Stats{"tags"}++;
    }
    if($Name eq "way"){
	$from_node=0;
    }
    if($Name eq "nd" ) {
	my $to_node   =  $Attr{"ref"};
	if ( $from_node &&
	     defined($Nodes{$from_node}) &&
	     defined($Nodes{$to_node})
	     ) {
	    my ($lat1,$lon1)=split(",",$Nodes{$from_node});
	    my ($lat2,$lon2)=split(",",$Nodes{$to_node});
	    my $angle = angle_north_relative(
				       { lat => $lat1 , lon => $lon1 },
				       { lat => $lat2 , lon => $lon2 });
	    push (@{$read_osm_segments},[$lat1,$lon1,$lat2,$lon2,$angle]);
    }
	$from_node = $to_node;
    }
}

# Function is called whenever an XML tag is ended
#----------------------------------------------
sub DoEnd(){
    my ($Expat, $Element) = @_;
    my $ID = $MainAttr{"id"};

    if($Element eq "node"){
	my $node={};
	$node->{"lat"} = $MainAttr{"lat"};
	$node->{"lon"} = $MainAttr{"lon"};

	if ( $AREA_FILTER->inside($node) ) {
	    $Nodes{$ID} = sprintf("%f,%f",$MainAttr{lat}, $MainAttr{lon});
	    foreach(keys(%Tags)){
		$node->{$_} = $Tags{$_};
	    }
	}
    }

    if($Element eq "segment"){
	my $from = $MainAttr{"from"};
	my $to   = $MainAttr{"to"};
	if ( defined($Nodes{$from}) && defined($Nodes{$to}) ) {
	    $Segments{$ID}{"from"} = $from;
	    $Segments{$ID}{"to"} = $to;
	}
    }

    if($Element eq "way"){
	if ( @WaySegments ) {
	    foreach my $seg_id( @WaySegments ){ # we only have the needed ones in here
	    }
	}
    }
}

# Function is called whenever text is encountered in the XML file
#----------------------------------------------
sub DoChar(){
    my ($Expat, $String) = @_;
}

# --------------------------------------------
sub read_osm_file($;$) { # Insert Segments from osm File
    my $filename = shift;
    my $bounds  = shift;

    print("Reading OSM Segment from File $filename\n") if $VERBOSE || $DEBUG;
    if ( file_needs_re_generation($filename,"$filename.storable")) {
	print "$filename:	".(-s $filename)." Bytes\n" if $DEBUG;

	print STDERR "Parsing file: $filename\n" if $DEBUG;
	my $p = new XML::Parser( Handlers => {
	    Start => \&DoStart,
	    End => \&DoEnd,
	    Char => \&DoChar,
			     },
				 ErrorContext => 10,
	    );

	my $fh = data_open($filename);
	if (not $fh) {
	    print STDERR "WARNING: Could not open osm data from $filename\n";
	    return;
	}
	my $content;
	eval {
	    $content = $p->parse($fh);
	};
	if (not $p) {
	    print STDERR "WARNING: Could not parse osm data from $filename\n";
	    return;
	}
	#warn Dumper(\$read_osm_segments);
	Storable_save($filename,$read_osm_segments);
    } else {
	$read_osm_segments=Storable_load($filename);
    }
	return($read_osm_segments);
}

# -------------------------------------------------------

1;

=head1 NAME

Geo::OSM::SegmentList

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
