DIRS = app1.d kern1.d process1.d

all:
	set -x; for D in $(DIRS); do cd $$D; make install; ./src/bootstrap; cd ..; done

