PREFIX?=/usr/local

all: videoToQuad

videoToQuad: Makefile videoToQuad.cpp perspectiveTransform/perspectiveTransform.o
	g++ -std=c++11 -g videoToQuad.cpp -I./perspectiveTransform perspectiveTransform/perspectiveTransform.o -o videoToQuad -ltiff -lpthread -lavcodec -lavformat -lswscale -lavutil -lz

clean:
	${MAKE} PREFIX=${PREFIX} -C perspectiveTransform clean
	rm videoToQuad

install: 
	install %D videoToQuad ${PREFIX}/bin

uninstall: 
	rm ${PREFIX}/bin/videoToQuad


.PHONY: all clean install uninstall
