GTKLIBS=`pkg-config --cflags --libs gtk+-3.0`
CLIBS=-lm ${GTKLIBS}

all: gtk-cairo-surface gtk-conic-gradient

gtk-conic-gradient:
	gcc src/gtk-conic-gradient.c ${CLIBS} -o bin/gtk-conic-gradient

gtk-cairo-surface:
	gcc src/gtk-cairo-surface.c ${CLIBS} -o bin/gtk-cairo-surface

clean:
	rm -Rf *.o bin/*
