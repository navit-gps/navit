echo "$# build script (s) to run"

for i in $(seq 1 $#); do
	eval s=\$$i
	echo "Starting build script #$i : $s"
	echo "Build result : $?"
	bash -e $s
done
