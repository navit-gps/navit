#!/bin/bash
set -e

# Let's check if there is files in the import queue
[ -d po/import_queue/ ] || exit 0

# Let's fix the headers of the .po files in the import queue
for i in po/import_queue/*.po; do
	b=`basename $i`;
	po=${b#*-};
	code=${po%.*}
        git checkout -b i18n/$code
	lname=`head -n1 ${i} | sed 's/# \(.*\) translation.\{0,1\} for navit/\1/'`
        if [[ $lname == "" ]]; then
                echo "Cannot find the language name in the header of $i"
                exit 1
        fi
	d=`date +"%Y"`
        echo "# ${lname} translations for navit" > po/${po}.header
        echo "# Copyright (C) 2006-${d} The Navit Team" >> po/${po}.header
        echo "# This file is distributed under the same license as the navit package." >> po/${po}.header
        echo "# Many thanks to the contributors of this translation:" >> po/${po}.header
	# Build a clean list of the contributors
	IFS=$'\n'
	echo "Downloading https://translations.launchpad.net/navit/trunk/+pots/navit/${code}/+details"
	contributors=`wget -q https://translations.launchpad.net/navit/trunk/+pots/navit/${code}/+details -O - | egrep '^              <a href=".+?" class="sprite person">'`
        for user in $contributors; do
                url=`echo $user|cut -d'"' -f2`
                name=`echo $user|cut -d'>' -f2|cut -d'<' -f1`
                echo "# $name $url" >> po/${po}.header
        done
        echo ''  >> po/${po}.header
        echo 'msgid ""'  >> po/${po}.header

        # We remove two tags that just generate noise
        sed -i '/X-Launchpad-Export-Date/d' ${i}
        sed -i '/X-Generator/d' ${i}
        sed -i '/POT-Creation-Date/d' ${i}

        # Let's put the translation from launchpad without the header
        mv po/${po}.header po/${po}.in
        sed '1,/msgid ""/ d' ${i} >> po/${po}.in

	git add po/${po}.in && rm $i
	git commit -m "Updated ${lname} translation from launchpad" po/${po}.in
        git push --set-upstream origin i18n/${code}
done
