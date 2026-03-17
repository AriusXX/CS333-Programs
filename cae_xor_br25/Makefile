CC = gcc
DEBUG = -g
DEFINES = 
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
		  -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
		  -Wdeclaration-after-statement -Wno-return-local-addr -Wunsafe-loop-optimizations \
		  -Wuninitialized -Werror
PROG1 = cae-xor
PROG2 = mystat
PROGS = $(PROG1) $(PROG2)

all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^

$(PROG1).o: $(PROG1).c
	$(CC) $(CFLAGS) -c $<

$(PROG2): $(PROG2).o
	$(CC) $(CFLAGS) -o $@ $^

$(PROG2).o: $(PROG2).c
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa cae_xor_${LOGNAME}.tar.gz *.[c] [mM]akefile


