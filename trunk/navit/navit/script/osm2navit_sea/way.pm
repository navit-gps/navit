#!/usr/bin/perl -w
#

package way;

use strict;
use warnings;

sub new {
	my($class, $way_id) = @_;
	my $self = bless({}, $class);

	$self->{id} = $way_id;
	$self->{first_node} = $self->{last_node} = $self->{prev} = $self->{next} = $self->{size} = 0;

	$self->{type} = "outer";

	$self->{is_alive} = 1;

	$self->{nodes}=[];

	return $self;
}

# id of the way
sub id {
	my $self = shift;

	return $self->{id};
}

# way killed or not
sub is_alive {
	my $self = shift;

	return $self->{is_alive};
}

# way type: outer or inner multipolygon => island or sea
sub type {
	my $self = shift;
	
	if( @_ ) {
		$self->{type} = shift;
	}

	return $self->{type};
}

# distroy way -> remove @nodes to free memory
sub distroy {
	my $self = shift;

	$self->{nodes}=[];
	$self->{is_alive} = 0;
}

# first node of the way
sub first_node {
	my $self = shift;
	if( @{$self->{nodes}} ) {
		return @{$self->{nodes}}[0];
	} else {
		return 0;
	}
}

# last node the way
sub last_node {
	my $self = shift;
	if( @{$self->{nodes}} ) {
		return @{$self->{nodes}}[ $self->{size} -1 ];
	} else {
		return 0;
	}
}

# next way
sub next {
	my $self = shift;
	if( @_ ) {
		$self->{next} = shift;
	}
	return $self->{next};
}

sub prev {
	my $self = shift;
	if( @_ ) {
		$self->{prev} = shift;
	}
	return $self->{prev};
}

# number of node in the way
sub size {
	my $self = shift;
	return $self->{size};
}

#nodes
sub nodes {
	my $self = shift;
	if( @_ ) {

		@{$self->{nodes}}=();
		my $prev=0;
		foreach (@_) {
			unless ($_ eq $prev) {
				$prev=$_;
				push ( @{$self->{nodes}} , $_);
			}
		}

		$self->{size} = scalar(@{$self->{nodes}});
	}

	return @{$self->{nodes}};
}

1;
