
CC = gcc
DEBUG = -g
DEFINES = 
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
		  -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
		  -Wdeclaration-after-statement -Wno-return-local-addr -Wunsafe-loop-optimizations \
		  -Wuninitialized -Werror -I~/Classes/cs333/src/md5
LDFLAGS = -lmd
PROG1 = arvik-md4
PROGS = $(PROG1)

all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PROG1).o: $(PROG1).c
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa cae_xor_${LOGNAME}.tar.gz *.[c] [mM]akefile


