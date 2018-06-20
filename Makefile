###############################################################################
##
## Makefile for XSLT CGI processor
## Requires Expat and Sablotron
##
## cej@nightwolf.org.uk
## November 2004
##
###############################################################################

###############################################################################
## No user definable parts beyond this point

LIBS=-lsablot -lexpat

CFLAGS=-O2 -W -Wall -DXSL_CONFIG="\"$(XSL_CONFIG)\""
LDFLAGS=$(LIBS)

xslt: xslt.o

clean:
	rm -f xslt xslt.o

install:
	strip xslt
	cp xslt /home/cej/www/cej/cgi-bin/xslt2
	chmod 755 /home/cej/www/cej/cgi-bin/xslt2
