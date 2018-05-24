This is the code used to build the image available at https://hub.docker.com/r/navit/tomtom-build-image/

If you want to build and use that image locally, you can do the following:
docker build . -t navit/tomtom-build-image
docker run -ti -v /tmp:/output navit/tomtom-build-image /bin/bash /entrypoint.sh

The resulting build will be in your /tmp/ folder.
