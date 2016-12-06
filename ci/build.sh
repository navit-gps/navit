set -e

echo "$# build script (s) to run"

for i in $(seq 1 $#); do
	eval s=\$$i
	echo "Starting build script #$i : $s"
	bash -e $s
done
