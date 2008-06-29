#! /usr/bin/perl
# Do a
# svn co http://svn.openstreetmap.org/applications/utils/perl_lib/Geo
# and
# svn co http://svn.openstreetmap.org/applications/utils/perl_lib/Utils
# first
# Example usage:
# ./border_follower.pl 10359135 Germany 2 country state
use Geo::OSM::APIClientV5;
use Data::Dumper;
$first_wayid=$ARGV[0];
$name=$ARGV[1];
$required_admin_level=$ARGV[2];
$type=$ARGV[3];
$alt_type=$ARGV[4];
$revers=0;
$api=new Geo::OSM::APIClient(api=>'http://www.openstreetmap.org/api/0.5');
$wayid=$first_wayid;
$path="$first_wayid";
sub error
{
	my ($message)=@_;
	$node=$api->get_node($last);
	print "$message at  $last $node->{lat} $node->{lon}\n";
	$latl=$node->{lat}-0.01;
	$lath=$node->{lat}+0.01;
	$lonl=$node->{lon}-0.01;
	$lonh=$node->{lon}+0.01;
	system("wget -O error.osm http://www.openstreetmap.org/api/0.5/map?bbox=$lonl,$latl,$lonh,$lath ; java -jar ~/map/osm/josm/josm-latest.jar error.osm --selection=id:$last");
	exit(1);
}

do {
	print "Following $wayid\n";
	$way=$api->get_way($wayid);
#	print Dumper($way->{'tags'});

	$nodes=$way->nodes;
	$tags = $way->tags;
	$reverse=0;
#	print Dumper($tags);
	while( my($k,$v) = splice @{$tags}, 0, 2 ) {
		if (($k eq "left:$type" || $k eq "left:$alt_type") && $v eq $name) {
			$reverse=1;
		}
	}
	if ($reverse) {
		$first=$nodes->[$#$nodes];
		$last=$nodes->[0];
	} else {
		$first=$nodes->[0];
		$last=$nodes->[$#$nodes];
	}
	$ways=$api->get_node_ways($last);

#	print "first=$first\n";
#	print "last=$last\n";

	print "$#$ways Ways\n";
	if ($#$ways <= 0) {
		error "End";
	}
	$lastid=$wayid;
	$count=0;
	foreach $way (@$ways) {
		$newid=$way->{'id'};
		print "Way $newid\n";
		my $tags = $way->tags;
		$match=0;
		$boundary=0;
		$admin_level=0;
		while( my($k,$v) = splice @{$tags}, 0, 2 ) {
			print "tag: $k=$v\n";
			if (($k eq "left:$type" || $k eq "right:$type") && $newid != $lastid) {
				if ($v eq $name) {
					$match=1;
				} else {
					$neighbors{$v}=$newid;
				}
			}
			if (($k eq "left:$alt_type" || $k eq "right:$alt_type") && $v eq $name && $newid != $lastid) {
				print "Warning: $k in $newid\n";
				$match=1;
			}
			if ($k eq "boundary") {
				if ($boundary == 0 && $v eq "administrative") {
					$boundary=1;
				} else {
					$boundary=2;
				}
			}
			if ($k eq "admin_level") {
				if ($admin_level == 0 && $v eq $required_admin_level) {
					$admin_level=1;
				} else {
					$admin_level=2;
				}
			}
		}
		print "match=$match\n";
		if ($match) {
			if ($boundary != 1 ) {
				print "boundary $boundary wrong at $newid\n"
			}
			if ($admin_level != 1) {
				print "admin_level $admin_level wrong at $newid\n"
			}
			$wayid=$newid;
			$count++;
		}
	}
	if ($count != 1) {
		error "Count: $count != 1" 
	} else {
		$path="$path $wayid";
	}
} while ($wayid != $first_wayid);
print "End reached\n";
print "Path $path\n";
while (($key,$value)=each(%neighbors)) {
	print "Neighbor $key $value\n";
}
#print Dumper($ways);
