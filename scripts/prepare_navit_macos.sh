# Use this script to build navit on a Mac
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install glib gtk+ gettext libpng protobuf-c cmake librsvg imagemagick gpsd
brew services start gpsd

#TODO: this needs to be adapted for the PR: git clone https://github.com/navit/navit.git
git clone https://github.com/OLFDB/navit.git
cd navit

#TODO: this needs to be adapted for the PR: remove
git checkout macosbuild
git fetch
#remove end

mkdir build
cd build
cmake ../contrib/macos/
open /usr/local/bin/navit.app
