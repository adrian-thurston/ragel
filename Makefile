DIRS = public cases
GENF = genf.d/app1

all:
	set -x; for D in $(DIRS); do cd $$D; make; cd ..; done
	set -x; for D in $(GENF); do cd $$D; make install; ./src/bootstrap; cd ../..; done
