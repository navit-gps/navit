if [ -z "$CIRCLE_BUILD_NUM" -o "$CIRCLE_PROJECT_USERNAME" != "navit-gps" ] ; then
  exit
fi

if ! git log -n 1 ; then
  echo "This script should be run from the versioned directory"
  exit 1
fi

if [ "$CIRCLE_BRANCH" != "trunk" ] ; then
  exit
fi

TAG=R$(( 5658 + $CIRCLE_BUILD_NUM ))

git log -1 --format="%H %d" | grep 'tag: R' 

if [ $? -eq 0 ] ; then
  echo "This commit is already tagged."
  exit
fi

if [ "$1" == "prepare" ] ; then
  git tag $TAG
  exit
fi

if [ "$1" == "push" ] ; then
  git push origin $TAG
fi

