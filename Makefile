# vegabar

include config.mk

SRC = drw.c vegabar.c util.c
OBJ = ${SRC:.c=.o}

all: options vegabar

options:
	@echo vegabar build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

vegabar: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f vegawm ${OBJ} vegawm-${VERSION}.tar.gz

dist: clean
	mkdir -p vegabar-${VERSION}
	cp -R Makefile config.mk\
		drw.h ${SRC} vegabar-${VERSION}
	tar -cf vegabar-${VERSION}.tar vegabar-${VERSION}
	gzip vegabar-${VERSION}.tar
	rm -rf vegabar-${VERSION}


.PHONY: all options clean dist 
