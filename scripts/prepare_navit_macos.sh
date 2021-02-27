# Use this script to build navit on a Mac 
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install glib gtk+ gettext libpng 
brew install protobuf-c
brew install cmake
brew install librsvg
git clone https://github.com/OLFDB/navit.git
cd navit
git checkout macosbuild
git fetch
mkdir build
cd build
cmake -Dbinding/python=false ../
make -j
make
make install
make install
