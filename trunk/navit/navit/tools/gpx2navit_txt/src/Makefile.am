bin_PROGRAMS = gpx2navit_txt 

gpx2navit_txt_SOURCES = \
	emess.h \
	geod_for.c \
	geod_inv.c \
	geod_set.c \
	geodesic.h \
	gpx2navit_txt.h \
	main.c \
	misc.c \
	utils.c \
	parser.c \
	elementControl.c\
	setwpt.c \
	setpath.c \
	setmeta.c \
	errorcode.h

INCLUDES= -I$(srcdir)

AM_CFLAGS =  -g 

debian-dist:
	dpkg-buildpackage -rfakeroot -d -tc -us -uc

