tar xfz ~/assets/cov-analysis-linux64-7.6.0.tar.gz
export PATH=~/navit/cov-analysis-linux64-7.6.0/bin:$PATH

mkdir bin && cd bin
cov-build --dir cov-int cmake ../
cov-build --dir cov-int make -j32
tar czvf navit.tgz cov-int
