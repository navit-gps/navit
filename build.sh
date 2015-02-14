wget http://sd-55475.dedibox.fr/cov-analysis-linux64-7.6.0.tar.gz
tar xfz cov-analysis-linux64-7.6.0.tar.gz
export PATH=~/navit/cov-analysis-linux64-7.6.0/bin

mkdir bin && cd bin
cov-build --dir cov-int cmake ../
cov-build --dir cov-int make
tar czvf navit.tgz cov-int
