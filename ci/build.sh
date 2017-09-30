set -e

echo "$# build script (s) to run"

for i in $(seq 1 $#); do
	eval s=\$$i
	echo "Starting build script #$i : $s"
	if [ ! -z $CIRCLE_ARTIFACTS ]; then
		set -o pipefail
		bash -e $s 2>&1 | tee $CIRCLE_ARTIFACTS/${i}.log
	else
		bash -e $s
	fi
done
