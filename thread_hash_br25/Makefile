CC = gcc
DEBUG = -g
DEFINES = 
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls \
		  -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
		  -Wdeclaration-after-statement -Wno-return-local-addr -Wunsafe-loop-optimizations \
		  -Wuninitialized -Werror -Wno-unused-parameter -Wno-string-compare -Wno-stringop-overflow \
		  -Wno-stringop-overread -Wno-stringop-truncation
LDFLAGS = -lcrypt
PROG1 = thread_hash
PROGS = $(PROG1)

all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PROG1).o: $(PROG1).c
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa thread_hash_${LOGNAME}.tar.gz *.[c] [mM]akefile


