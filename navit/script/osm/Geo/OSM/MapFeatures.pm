##################################################################
package Geo::OSM::MapFeatures;
##################################################################

use Exporter;
@ISA = qw( Exporter );
use vars qw(@ISA @EXPORT @EXPORT_OK %EXPORT_TAGS $VERSION);
@EXPORT = qw( );

use strict;
use warnings;

use HTTP::Request;
use File::Basename;
use File::Copy;
use File::Path;
use Getopt::Long;
use HTTP::Request;
use Storable ();
use Data::Dumper;

use Utils::File;
use Utils::Debug;
use Utils::LWP::Utils;
use XML::Parser;
use XML::Simple;

my $self;

# ------------------------------------------------------------------
sub style($){
    my $self = shift;

}

# ------------------------------------------------------------------
# load the complete MapFeatures Structure into memory
sub load($;$){
    my ($class, $filename) = @_;
    #$filename ||= '../../freemap/freemap.xml';
    $filename ||= './map-features.xml';
    print("Loading Map Features from  $filename\n") if $VERBOSE || $DEBUG;
    print "$filename:	".(-s $filename)." Bytes\n" if $DEBUG;
    print STDERR "Parsing file: $filename\n" if $DEBUG;

    my $fh = data_open($filename);
    if (not $fh) {
	print STDERR "WARNING: Could not open osm data from $filename\n";
	return;
    }
    my $self = XMLin($fh);

    if (not $self) {
	print STDERR "WARNING: Could not parse osm data from $filename\n";
	return;
    }

    #delete $self->{defs}->{symbol};
    #warn Dumper(\$self->{defs});
    #warn Dumper(\$self->{data});
    #warn Dumper(\$self->{rule});
    #warn Dumper(keys %{$self});
    #warn Dumper(%{$self});

    bless($self,$class);
    return $self;
}

# ------------------------------------------------------------------
# Load icons into memory nad create a PDF Version out of it
sub load_icons($$){
    my $self = shift;
    my $PDF  = shift;
    die "No PDF Document defined\n" unless $PDF;

    print STDERR "load_icons():\n" if $DEBUG;
#    print STDERR Dumper(\$self);

    for my $rule ( @{$self->{rule}} ) {
	my $img = $rule->{style}->{image};
	next unless defined $img;
	$img =~s/^images\///;
	my $img_filename;
	for my $path ( qw( ../../freemap/images
			   ../../map-icons/square.small
			   ../../map-icons/square.big
			   ../../map-icons/classic.big
			   ../../map-icons/classic.small
			   ../../map-icons/nick
			   ../../map-icons/classic/other
			   ) ) {
	    $img_filename = "$path/$img";
	    if ( -s $img_filename ) {
		my $png = $PDF->image_png($img_filename);
		$rule->{png}=$png;
		#print STDERR "image: $img_filename\n";
		last;
	    }
	}

	if ( ! $rule->{png} ) {
	    warn "missing $img\n";
	}
	#print STDERR "rule: ".Dumper(\$rule);
#	print STDERR "condition: ".Dumper(\$rule->{condition});
	my $condition = $rule->{condition};
#	print STDERR "image: $img_filename\t";
	print STDERR "     #condition: $condition->{k}=$condition->{v}\t";
	printf STDERR "scale: %d-%d",
	    ($rule->{style}->{scale_max}||0),
	    ($rule->{style}->{scale_min}||0);
	print STDERR "get_icon() image: $img\n";

    }
}

# ------------------------------------------------------------------
sub get_icons($$$){
    my $self = shift;
    my $rule_line= shift;
    my $scale = shift;
    return undef if $rule_line =~ m/^[\s\,]*$/;
#    return undef if $rule_line eq ",";

    my ($dummy1,$dummy2,$attr) = split(",",$rule_line,3);
    my %attr;
    foreach my $kv ( split(",",$attr)){
	my ($k,$v)=split("=",$kv);
	$attr{$k}=$v;
    }


    print STDERR "get_icon($attr)\n";

    my $png =undef;
    for my $rule ( @{$self->{rule}} ) {
	my $img = $rule->{style}->{image};
	next unless $img;

	my $condition = $rule->{condition};
#	print STDERR "condition: $condition->{k}=$condition->{v}\n";
	if ( defined ( $attr{scale_max}) &&
	     $scale > $attr{scale_max}) {
	    next;
	}

	if ( defined( $attr{ $condition->{k}}) &&
	     $attr{ $condition->{k}} eq $condition->{v} ) {
	    print STDERR "condition: $condition->{k}=$condition->{v}\n";
	    print STDERR "get_icon() image: $img\t";
	    $png = $rule->{png};
	}

	return $png if $png;
    }
    return undef;
}
1;

=head1 NAME

Geo::OSM::MapFeature

=head1 DESCRIPTION

Load the MapFeatures.xml into memory

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

Jörg Ostertag (MapFeatures-for-openstreetmap@ostertag.name)

=head1 SEE ALSO

http://www.openstreetmap.org/

=cut
