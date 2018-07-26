DIRS = public cases genf

all:
	set -x; for D in $(DIRS); do cd $$D; make; cd ..; done
