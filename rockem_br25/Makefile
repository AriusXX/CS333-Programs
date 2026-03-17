CC = gcc
DEBUG = -g
DEFINES = 
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
		  -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
		  -Wdeclaration-after-statement -Wno-return-local-addr -Wunsafe-loop-optimizations \
		  -Wuninitialized -Werror -Wno-unused-parameter -Wno-string-compare -Wno-stringop-overflow \
		  -Wno-stringop-overread -Wno-stringop-truncation
PROG1 = rockem_server
PROG2 = rockem_client
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
	tar cvfa rockem_${LOGNAME}.tar.gz *.[c] [mM]akefile


