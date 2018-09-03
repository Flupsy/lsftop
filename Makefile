# Replace with the location of your LSF installation.
#
LSFDIR=/wherever/LSF/is

CC=gcc
#CFLAGS=-I$(LSFDIR)/include -ggdb3 -Wall
CFLAGS=-I$(LSFDIR)/include -O6 -s
LDFLAGS=-L$(LSFDIR)/linux2.6-glibc2.3-x86_64/lib -lbat -llsf -lncurses -lnsl -lm -lrt
OFILES=display.o error.o lsftop.o help.o options.o kaboom.o dotfile.o info.o

lsftop: $(OFILES)
	$(CC) $(CFLAGS) -o lsftop $(OFILES) $(LDFLAGS)

clean:
	rm -f $(OFILES) lsftop
