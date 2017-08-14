set -e

echo "$# build script (s) to run"

for i in $(seq 1 $#); do
	eval s=\$$i
	echo "Starting build script #$i : $s"
	if [ ! -z $CIRCLE_ARTIFACTS ]; then
		bash -e $s 2>&1 | tee $CIRCLE_ARTIFACTS/${s}.log
	else
		bash -e $s
	fi
done
