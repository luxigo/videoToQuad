PREFIX?=/usr/local

all: perspectiveTransform videoToQuad

videoToQuad: videoToQuad.cpp perspectiveTransform/perspectiveTransform.o
	g++ -g videoToQuad.cpp -I./perspectiveTransform perspectiveTransform/perspectiveTransform.o -o videoToQuad -ltiff -lpthread

clean:
	rm *.o videoToQuad

install: 
	install %D videoToQuad ${PREFIX}/bin

uninstall: 
	rm ${PREFIX}/bin/videoToQuad


.PHONY: all perspectiveTransform clean
