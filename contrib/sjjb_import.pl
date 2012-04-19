#!/usr/bin/perl -w

# Convert the map icons from http://www.sjjb.co.uk/mapicons/
# for use by navit - we want the "Positive" variant and we want
# a different color for each section.
# WARNING: EXISTING FILES WILL BE OVERWRITTEN WITHOUT HESITATION!

# Usage:
#   sjjb_import.pl [source-dir] [dest-dir]

# (C)opyright 2012 Lutz Möhrmann
# License: GPL v2

### config #########################################################

# TODO: Set to 1 to get the "Negative" version as shown here:
# http://www.sjjb.co.uk/mapicons/contactsheet
# The first value from $colors will be used as background in this
# case, otherwise it's the foreground
#$negative = 0;

# TODO: Foreground color if $negative is set.
$fg_color = 'ffffff';

# Use the second value from $colors to produce a tiny border
$use_border = 0;

# Some icons in place_of_worship/ (and possibly other directories)
# contain a drop-shadow. Set to 1 to keep it.
$use_shadow = 0;

# TODO: produce "White Glow"
#$use_glow = 1;

# Format is main-color, border-color
%colors = (
	accommodation	 => ['00c08f', '226282'],
	amenity		 => ['734A08', '825d22'],
	barrier		 => ['666666', '808080'],
	education	 => ['39ac39', '208020'],
	food		 => ['c02727', '815c21'],
	health		 => ['da0092', '822262'],
	landuse		 => ['999999', '666666'],
	money		 => ['000000', '404040'],
	place_of_worship => ['5e8019', '7a993d'],
	poi		 => ['990808', '802020'],
	power		 => ['8e7409', '806d20'],
	shopping	 => ['ad30c0', '762282'],
	sport		 => ['39ac39', '208020'],
	tourist		 => ['b26609', 'bf8830'],
	transport	 => ['0089cd', '206080'],
	water		 => ['0050da', '204380'],
);

# Select/rename icons. The first string is the source filename (with path,
# but w/o .svg suffix), the second one is the destination filename. If omited,
# the source fn is used.
@icons = (
	['accommodation/shelter2', 'shelter'],

	['amenity/library'],
	['amenity/town_hall2', 'townhall'],

#	['barrier/', ''],

#	['education/', ''],

	['food/biergarten'],
	['food/drinkingtap', 'drinking_water'],

	['health/hospital_emergency', 'emergency'],

#	['landuse/', ''],

#	['money/', ''],

	['place_of_worship/bahai3', 'bahai'],
	['place_of_worship/buddhist3', 'buddhist'],
#	['place_of_worship/christian3', 'christian'],  # that's "church"
	['place_of_worship/hindu3', 'hindu'],
	['place_of_worship/islamic3', 'islamic'],
	['place_of_worship/jain3', 'jain'],
	['place_of_worship/jewish3', 'jewish'],
	['place_of_worship/pagan3', 'pagan'], # not there (yet)
	['place_of_worship/pastafarian3', 'pastafarian'], # not there (yet)
	['place_of_worship/shinto3', 'shinto'],
	['place_of_worship/sikh3', 'sikh'],
	['place_of_worship/taoist3', 'taoist'], # not there (yet)
	['place_of_worship/unknown', 'worship'],

#	['poi/', ''],

#	['power/', ''],

#	['shopping/', ''],

	['sport/leisure_centre', 'sport'],

	['tourist/casino'],
	['tourist/castle2', 'castle'],
	['tourist/memorial'],
	['tourist/picnic'],
	['tourist/theatre', 'theater'],

	['transport/bus_stop2', 'bus_stop'],
	['transport/daymark'], # not there (yet)
	['transport/rental_car', 'car_rent'],

	['shopping/car', 'car_dealer'],
	['shopping/clothes', 'shop_apparel'],
	['shopping/computer', 'shop_computer'],
	['shopping/department_store', 'shop_department'],

	['water/dam'],
);

### /config ########################################################

use File::Basename;
use XML::Twig;

@fg_patterns= ("fill:white:1", "stroke:white:1", "fill:#ffffff:1", "stroke:#ffffff:1");
@bg_patterns= ("fill:#111111:1", "fill:#111:1");
@brd_patterns= ("stroke:#eeeeee:1", "stroke:#eee:1");
@sh_patterns= ('fill:#ffffff:0.25', 'stroke:#ffffff:0.25');

$dbg_level= 1;

sub dbg {
	my ($lvl, $text)= @_;
	if ($lvl <= $dbg_level)
		{ print STDERR $text; }
}

sub change {
	my ($value, $patterns, $color, $transparent)= @_;
	&dbg(3, "change('$$value', , '$color', '$transparent')\n");
	foreach $p (@{$patterns}) {
		my ($k, $v, $a)= split(':', $p);
		$o= $opacity;
		if ($$value =~ m/\Aopacity:([.0-9]+)/i)
			{ $o*= $1; }
		elsif ($$value =~ m/;opacity:([.0-9]+)/i)
			{ $o*= $1; }
		if ($$value =~ m/$k-opacity:([.0-9]+)/i)
			{ $o*= $1; }
		if ($o >= 0.5) # we don't want half transparent stuff like in tourist/memorial or amenity/town_hall2
			{ $o= 1; }
		&dbg(4, "p='$p' k='$k' v='$v' a='$a' o='$o'\n");
		if ( !($$value =~ m/$k:$v/i)
		  or ((defined $a) and ($a != $o)) )
			{ next; }
		if (!$transparent) {
			&dbg(4, "changing color to '$color'\n");
			$$value =~ s/$k:$v/$k:$color/i;
		} else {
			&dbg(4, "changing opacity to '0'\n");
			if ($$value =~ m/$k-opacity:[0-9]/i) {
				$$value =~ s/$k-opacity:[0-9]/$k-opacity:0/i;
			} else {
				$$value =~ s/$k:$v/$k:$v;$k-opacity:0/i;
			}
		}

	}
}

sub g_handler {
	my $att= $_->{'att'};
	if (!defined $att->{'style'})
		{ return; }

	if ($att->{'style'} =~ m/[^-]?opacity:([.0-9]+)/i)
		{ $opacity= $1; }
}

sub handler {
	my $att= $_->{'att'};
	if (!defined $att->{'style'})
		{ return; }

	&dbg(2, "  in:  '$att->{'style'}'\n");
	my $fg= $negative ? $fg_color : $color1;
	&change(\$att->{'style'}, \@sh_patterns, $fg, !$use_shadow);
	&change(\$att->{'style'}, \@fg_patterns, $fg, 0);
	&change(\$att->{'style'}, \@bg_patterns, $color1, !$negative);
	&change(\$att->{'style'}, \@brd_patterns, $color2, !$use_border);
	&dbg(2, "  out: '$att->{'style'}'\n\n");
}

sub get_arg {
	my ($no, $q)= @_;
	if ($ARGV[$no]) {
		return $ARGV[$no];
	} else {
		print $q."\n";
		$input= <STDIN>;
		chomp($input);
		return $input;
	}
}

$sjjb_dir= get_arg(0, "Directory holding the extracted svg's from http://www.sjjb.co.uk/mapicons/downloads");
$dest_dir= $ARGV[1] ? $ARGV[1] : dirname(__FILE__)."/../navit/xpm/";

foreach $i (@icons) {
	my ($path, $icon)= split('/', $i->[0]);
	my ($dest)= $i->[1] ? $i->[1] : $icon;
	$color1= '#'.$colors{$path}[0];
	$color2= '#'.$colors{$path}[1];
	$opacity= 1.0;

	my $in_fn= "$sjjb_dir/$path/$icon.svg";
	open(my $in_f, $in_fn) or die "Couldn't read '$in_fn': $!";
	my $decl= <$in_f>; chomp $decl; # save the '<?xml ...'-line
	my $in;
	while (<$in_f>) { $in.= $_; }
	close($in_f);
	&dbg(1, "file $in_fn: ".length($in)." bytes.\n");
	my $xml= XML::Twig->new(
		pretty_print => 'indented',
		twig_handlers => {
			'path' => \&handler,
			'rect' => \&handler,
			'circle' => \&handler,
			'svg:path' => \&handler,
			'svg:rect' => \&handler,
			'svg:circle' => \&handler,
		},
		start_tag_handlers => {
			'g' => \&g_handler,
			'svg:g' => \&g_handler,
		},
	);

	$xml->parse($in);

	my $out_fn= "$dest_dir/$dest.svg";
	open(my $out_f, "> $out_fn") or die "Couldn't write to '$out_fn': $!";
	print $out_f $decl."\n".$xml->sprint();

	$xml->purge;
}
