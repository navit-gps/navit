#! /usr/bin/perl

# This script parses the XML config file and checks if known itemgra elements are defined or not in each layout
# Run ./itemgra.pl 2>/dev/null to get only the result without parsing information messages
my $layout;
my $type,$types;
my $garmintypes;
my $order,$orders,@orders;
my @layouts;
my $input_stream;
my @input_stream_stack;
my $current_filename = "../navit_shipped.xml";
if (!open($input_stream,$current_filename)) {
	print(STDERR "Failed to open \"" . $current_filename . "\"\n");
	exit(1);
}
while(defined($input_stream)) {
	#printf("%d input streams in file recursion stack. Current input stream is " . $input_stream . "\n", $#input_stream_stack+1);
	while (<$input_stream>) {
		#print($_);
		if (/<layout name="([^"]*)"/) {
			$layout=$1;
			push(@layouts,$layout);
		}
		if (/<xi:include href="([^"]*)"/) {
			my $xml_include_file;
			$xml_include_file=$1;
			if ($xml_include_file =~ /^[\/\$]/) {	# PATH is absolute or starts with a variable... do not modify PATH
			}
			$xml_include_file =~ s@^@../@;	# Make PATH relative to ../ (navit XML config is in parent
			if ($xml_include_file =~ /.*maps\/\*\.xml$/) {	# Skip mapsets that use wildcards
			}
			else {
				push(@input_stream_stack, $input_stream);
				$input_stream = undef;
				print(STDERR "Opening included file \"" . $xml_include_file . "\"\n");
				if (!open($input_stream, $xml_include_file)) {
					$xml_include_file =~ s/\.xml$/_shipped.xml/;	# If file does not exist, try suffixing the filename with _shipped
					print(SDTERR "Opening included file \"" . $xml_include_file . "\"\n");
					if (!open($input_stream, $xml_include_file)) {
						print(STDERR "Failed to open \"" . $xml_include_file . "\"\n");
						exit(1);
					}
				}
				$current_filename = $xml_include_file;
				#printf("%d input streams in file recursion stack. Current input stream is " . $input_stream . "\n", $#input_stream_stack+1);
				next;
			}
		}
		if (/<itemgra item_types="([^"]*)"/) {
			$types=$1;
			if (/order="([^"]*)"/) {
				$order=$1;
				print(STDERR "Order $order found in \"" . $current_filename . "\"\n");
				foreach $type (split(",",$types)) {
					$result->{$type}->{$layout}->{$order}=1;
				}
			}
		}
	}
	close($input_stream);
	$input_stream = pop(@input_stream_stack);
	#print("Popped input stream " . $input_stream . "\n");
}
print(STDERR "Finished parsing navit XML config\n");
print(STDERR "Parsing item_def.h\n");
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
print(STDERR "Parsing ../map/garmin/garmintypes.txt\n");
open(IN,"../map/garmin/garmintypes.txt");
while (<IN>) {
	if (/^[0-9][^=]*=\s*([^,]*),.*$/) {
		$type=$1;
		print("Garmintype: " . $type . "\n");
		$garmintypes->{$type}->{"none"}=1;
	}
}
close(IN);

print(STDERR "Outputting results\n");
my $typefmt="%-30s";
my $layoutfmt="%10s";
printf($typefmt,"");
foreach $layout (@layouts) {
	printf($layoutfmt,$layout);
}
print("\n");
# Check which map items defined in ../item_def.h are actually handled by each layout
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

print("Analysis on ../map/garmin/garmintypes.txt:\n");
foreach $type (sort keys %$garmintypes) {
	if (!$result->{$type}->{"none"}) {
		print($type . " exists in garmintypes.txt and missing in itemdef.h\n");
	}
}