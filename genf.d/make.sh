root=`pwd`

for d in app1 kern1; do
	cd $d
	~/pkgs/pkgbuild/bin/autogen.sh
	./configure --prefix=$root/inst --with-subject=$HOME/pkgs/genf --with-aapl=$HOME/pkgs/aapl
	make
	make install
	./src/bootstrap
	cd ..
done
