root=`pwd`
cd app1

~/pkgs/pkgbuild/bin/autogen.sh
./configure --prefix=$root/inst --with-subject=$HOME/pkgs/genf --with-aapl=$HOME/pkgs/aapl
make
make install
./app1/bootstrap
