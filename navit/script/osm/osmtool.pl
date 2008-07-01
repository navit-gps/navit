#! /usr/bin/perl
use Geo::OSM::APIClientV5;
use Data::Dumper;

sub print_error
{
	print Dumper($api->{last_error});
	print 'Error:' . $api->{last_error}->rc . "\n";
}

sub print_entitiy
{
	my ($res)=@_;

	my ($xml);
	$xml=$res->xml;
	$xml=~s/>/>\n/g;
	print $xml;
}

sub remove_member
{
	my ($res,$type,$id)=@_;

	my (@members);
	my $found=0;
	foreach my $member (@{$res->members}) {
		if ($member->member_type eq $type && $member->ref eq $id) {
			$found++;
		} else {
			push(@members, $member);
		}
	}
	if ($found) {
		$res->set_members(\@members);
	}
	return $found;
}

sub cmd_delete
{
	my($type,$id)=@_;
	my($res);
	$res=$api->get($type,$id);
	if (!$api->delete($res)) {
		print_error();
		return 1;
	}
	return 0;
}

sub cmd_get
{
	my($type,$id)=@_;

	my($res,$xml);
	$res=$api->get($type,$id);
	print_entitiy($res);
}

sub cmd_relations
{
	my($type,$id)=@_;

	my($res,$xml);
	$res=$api->get($type,$id,"relations");
	print_entitiy($res);
}

sub cmd_remove_member_all
{
	my($type,$id)=@_;

	my($res);
	$res=$api->get($type,$id,"relations");
	if (!remove_member($res,$type,$id)) {
		print "Error:Member not found\n";
		return 1;
	}
	if (!$api->modify($res)) {
		print_error();
		return 1;
	}
	return 0;
}

sub cmd_reload
{
	my($type,$id)=@_;
	$res=$api->get($type,$id);
	if (!$api->modify($res)) {
		print_error();
		return 1;
	}
	return 0;
}

sub command
{
	my(@arg)=@_;

	if ($arg[0] eq 'delete') {
		cmd_delete($arg[1],$arg[2]);
	}
	if ($arg[0] eq 'get') {
		cmd_get($arg[1],$arg[2]);
	}
	if ($arg[0] eq 'relations') {
		cmd_relations($arg[1],$arg[2]);
	}
	if ($arg[0] eq 'remove-member-all') {
		cmd_remove_member_all($arg[1],$arg[2]);
	}
	if ($arg[0] eq 'reload') {
		cmd_reload($arg[1],$arg[2]);
	}
}

while (substr($ARGV[0],0,2) eq '--') {
	$expr=substr($ARGV[0],2);
	($key,$value)=split('=',$expr,2);
	$attr{$key}=$value;
	shift;
}
$api=new Geo::OSM::APIClient(api=>'http://www.openstreetmap.org/api/0.5',%attr);
command(@ARGV);
