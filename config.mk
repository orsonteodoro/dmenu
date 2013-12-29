# dmenu version
VERSION = 4.5-tip

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Xinerama, comment if you don't want it
#XINERAMALIBS  = -lXinerama
#XINERAMAFLAGS = -DXINERAMA

# includes and libs
#INCS = -I${X11INC}
#LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS}

# flags
CPPFLAGS = -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
CFLAGS   = -std=gnu99 -pedantic -Wall -Os -DUSE_WINAPI -DUSE_CYGWIN ${INCS} ${CPPFLAGS}
LDFLAGS  = -s -mwindows ${LIBS}

# compiler and linker
CC = cc
