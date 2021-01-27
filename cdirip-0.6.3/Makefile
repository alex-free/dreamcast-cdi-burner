#####################################
#                                   
#  Custom-written for making Debian package for CDIRip
#                                   
#####################################
 
CC=gcc

# Output filename
OUTPUT="cdirip"

# Source files
SRCS=cdirip.c buffer.c cdi.c common.c audio.c
# Output object files (*.o)
OBJECTS = ${SRCS:.c=.o}
# Headers (*.h)
HEADERS= ${SRCS:.c=.h}

RM = rm -f
MV = mv -f
CP = cp -f

.c.o:
	${CC} -c ${CFLAGS} $<
	
cdirip: ${OBJECTS}
	${CC} -o $@ ${OBJECTS} -lm
	
all: cdirip

clean:
	-${RM} ${OUTPUT} ${OBJECTS}

mrproper: clean

install: all
	${CP} ${OUTPUT} ${DESTDIR}/usr/bin                         # Line 25
	${CP} ${HEADERS} ${DESTDIR}/usr/include/cdirip                # Line 26