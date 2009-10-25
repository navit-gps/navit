#! /usr/bin/perl
my $layout;
my $type,$types;
my $order,$orders,@orders;
my @layouts;
open(IN,"../navit.xml");
while (<IN>) {
	if (/<layout name="([^"]*)"/) {
		$layout=$1;
		push(@layouts,$layout);
	}
	if (/<itemgra item_types="([^"]*)"/) {
		$types=$1;
		if (/order="([^"]*)"/) {
			$order=$1;
			foreach $type (split(",",$types)) {
				$result->{$type}->{$layout}->{$order}=1;
			}
		}
	}
}
close(IN);
open(IN,"../item_def.h");
while (<IN>) {
	if (/^ITEM2\([^,]*,(.*)\)/) {
		$type=$1;
		$result->{$type}->{"none"}=1;
	}
	if (/^ITEM\((.*)\)/) {
		$type=$1;
		$result->{$type}->{"none"}=1;
	}
}
close(IN);
my $typefmt="%-30s";
my $layoutfmt="%10s";
printf($typefmt,"");
foreach $layout (@layouts) {
	printf($layoutfmt,$layout);
}
printf("\n");
foreach $type (sort keys %$result) {
	$marker="";
	if (!$result->{$type}->{"none"}) {
		$marker="#";
	}
	printf($typefmt,$marker.$type);
	foreach $layout (@layouts) {
		@orders=sort keys %{$result->{$type}->{$layout}};
		$order="";
		if ($#orders >= 0) {
			$order="*";
		}
		printf($layoutfmt,$order);
	}
	printf("\n");
}
