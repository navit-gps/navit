# Let's check if there is files in the import queue
[ -d navit/navit/import_queue/ ] || exit 0

# Let's fix the headers of the .po files in the import queue
for i in navit/navit/import_queue/*.po; do
	b=`basename $i`;
	po=${b#*-};
	code=${po%.*}
	lname=`head -n1 ${i} | sed 's/# \(.*\) translation.\{0,1\} for navit/\1/'`
        if [[ $lname == "" ]]; then
                echo "Cannot find the language name in the header of $i"
                exit 1
        fi
	d=`date +"%Y"`
        echo "# ${lname} translations for navit" > navit-code/navit/po/${po}.header
        echo "# Copyright (C) 2006-${d} The Navit Team" >> navit-code/navit/po/${po}.header
        echo "# This file is distributed under the same license as the navit package." >> navit-code/navit/po/${po}.header
        echo "# Many thanks to the contributors of this translation:" >> navit-code/navit/po/${po}.header
	# Build a clean list of the contributors
	IFS=$'\n'
	echo "Downloading https://translations.launchpad.net/navit/trunk/+pots/navit/${code}/+details"
	contributors=`wget -q https://translations.launchpad.net/navit/trunk/+pots/navit/${code}/+details -O - | egrep '^              <a href=".+?" class="sprite person">'`
        for user in $contributors; do
                url=`echo $user|cut -d'"' -f2`
                name=`echo $user|cut -d'>' -f2|cut -d'<' -f1`
                echo "# $name $url" >> navit-code/navit/po/${po}.header
        done
        echo ''  >> navit-code/navit/po/${po}.header
        echo 'msgid ""'  >> navit-code/navit/po/${po}.header

        # We remove two tags that just generate noise
        sed -i '/X-Launchpad-Export-Date/d' ${i}
        sed -i '/X-Generator/d' ${i}
        sed -i '/POT-Creation-Date/d' ${i}

        # Let's put the translation from launchpad without the header
        mv navit-code/navit/po/${po}.header navit/navit/po/${po}.in
        sed '1,/msgid ""/ d' ${i} >> navit/navit/po/${po}.in

	# Yay, we should have a clean .po file now!
	git diff navit/navit/po/${po}.in
done
	
