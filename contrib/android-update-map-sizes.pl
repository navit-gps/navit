#!/usr/bin/perl
# Navit, a modular navigation system.
# Copyright (C) 2005-2008 Navit Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.


print STDERR "NavitMapDownloader.java map sizes update script.\n";
print STDERR "Please feed me NavitMapDownloader.java on stdin, get the updated file on stdout.\n";

while(<>){
	if(/^(.*new osm_map_values\(.*)"([\-0-9\.]+)"\s*,\s*"([\-0-9\.]+)"\s*,\s*"([\-0-9\.]+)"\s*,\s*"([\-0-9\.]+)"\s*,\s*([0-9L]+)\s*,\s*([0-9L]+.*\).*$)/i) {
		$prefix=qq($1 "$2", "$3", "$4", "$5",);
		$suffix=$7;
		$size="==err==";
		$curline="$_";
		do{
			print STDERR $curline;
			open IN, "curl --head -L http://maps.navit-project.org/api/map/?bbox=$2,$3,$4,$5|";
			while(<IN>) {
				$size="$1L" if(/^Content-Length:\s*(\d+)/)
			}
			close IN;
		} while($size eq "==err==");
		print "$prefix $size, $suffix\n";
	} else {
		print "$_";
	}
}
