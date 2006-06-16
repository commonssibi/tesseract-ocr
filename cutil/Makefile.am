SUBDIRS =
AM_CPPFLAGS = -I$(top_srcdir)/ccutil

EXTRA_DIST = \
    bitvec.h callcpp.h const.h danerror.h debug.h efio.h \
    emalloc.h freelist.h funcdefs.h general.h globals.h listio.h \
    minmax.h oldheap.h oldlist.h structures.h tessarray.h \
    tordvars.h util.h variables.h

noinst_LIBRARIES = libtesseract_cutil.a
libtesseract_cutil_a_SOURCES = \
    tessarray.cpp bitvec.cpp danerror.cpp debug.cpp efio.cpp \
    emalloc.cpp freelist.cpp globals.cpp listio.cpp oldheap.cpp \
    oldlist.cpp structures.cpp tordvars.cpp util.cpp variables.cpp
