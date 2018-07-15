##################################################################
package Geo::OSM::Write;
##################################################################

use Exporter;
@ISA = qw( Exporter );
use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);
@EXPORT = qw( write_osm_file );

use strict;
use warnings;

use Math::Trig;
use Data::Dumper;

use Geo::Geometry;
use Utils::Debug;
use Utils::File;
use Utils::Math;


# ------------------------------------------------------------------
sub tags2osm($){
    my $obj = shift;

    my $erg = "";
    for my $k ( keys %{$obj->{tag}} ) {
	my $v = $obj->{tag}{$k};
	if ( ! defined $v ) {
	    warn "incomplete Object: ".Dumper($obj);
	}
	#next unless defined $v;

	# character escaping as per http://www.w3.org/TR/REC-xml/
	$v =~ s/&/&amp;/g;
	$v =~ s/\'/&apos;/g;
	$v =~ s/</&lt;/g;
	$v =~ s/>/&gt;/g;
	$v =~ s/\"/&quot;/g;

	$erg .= "    <tag k=\'$k\' v=\'$v\' />\n";
    }
    return $erg;
}

sub write_osm_file($$) { # Write an osm File
    my $filename = shift;
    my $osm = shift;

    my $osm_nodes    = $osm->{nodes};
    my $osm_ways     = $osm->{ways};

    $osm->{tool} ||= "OSM-Tool";
    my $count_nodes    = 0;
    my $count_ways     = 0;

    my $generate_ways=$main::generate_ways;

    my $start_time=time();

    printf STDERR ("Writing OSM File $filename\n") if $VERBOSE >1 || $DEBUG>1;

    my $fh;
    if ( $filename eq "-" ) {
	$fh = IO::File->new('>&STDOUT');
	$fh or  die("cannot open STDOUT: $!");
    } else {
	$fh = IO::File->new(">$filename");
    }
    $fh->binmode(':utf8');

    print $fh "<?xml version='1.0' encoding='UTF-8'?>\n";
    print $fh "<osm version=\'0.5\' generator=\'".$osm->{tool}."\'>\n";
    if ( defined ( $osm->{bounds} ) ) {
	my $bounds = $osm->{bounds};
	my $bounds_sting = "$bounds->{lat_min},$bounds->{lon_min},$bounds->{lat_max},$bounds->{lon_max}";
	# -90,-180,90,180
	print $fh "   <bound box=\"$bounds_sting\" origin=\"OSM-perl-writer\" />\n";

    }

    # --- Nodes
    for my $node_id (  sort keys %{$osm_nodes} ) {
	next unless $node_id;
	my $node = $osm_nodes->{$node_id};
	my $lat = $node->{lat};
	my $lon = $node->{lon};
	unless ( defined($lat) && defined($lon)){
	    printf STDERR "Node '$node_id' not complete\n";
	    next;
	}
	print $fh "  <node id=\'$node_id\' ";
	print $fh " timestamp=\'".$node->{timestamp}."\' "
	    if defined $node->{timestamp};
	print $fh " changeset=\'".$node->{changeset}."\' "
	    if defined $node->{changeset};
	print $fh " lat=\'$lat\' ";
	print $fh " lon=\'$lon\' ";
	print $fh ">\n";
	print $fh tags2osm($node);
	print $fh "  </node>\n";
	$count_nodes++;
    }

    # --- Ways
    for my $way_id ( sort keys %{$osm_ways} ) {
	next unless $way_id;
	my $way = $osm_ways->{$way_id};
	next unless scalar( @{$way->{nd}} )>1;

	print $fh "  <way id=\'$way_id\'";
	print $fh " timestamp=\'".$way->{timestamp}."\'"
	    if defined $way->{timestamp};
	print $fh ">";

	for my $way_nd ( @{$way->{nd}} ) {
	    next unless $way_nd;
	    print $fh "    <nd ref=\'$way_nd\' />\n";
	}
	print $fh tags2osm($way);
	print $fh "  </way>\n";
	$count_ways++;

    }

    print $fh "</osm>\n";
    $fh->close();

    if ( $VERBOSE || $DEBUG ) {
	printf STDERR "%-35s:	",$filename;
	printf STDERR " Wrote OSM File ".
	    "($count_nodes Nodes, $count_ways Ways)";
	print_time($start_time);
    }

}

1;

=head1 NAME

Geo::OSM::Write

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
