CC=gcc
INCLUDE=include
CFLAGS_AS=-fsanitize=address
#CFLAGS_AS=
CFLAGS=-I$(INCLUDE) -Wall -ggdb -fno-omit-frame-pointer $(CFLAGS_AS)

BT_LIBS=-lbluetooth
DB_LIBS=-lsqlite3
BINDIR=bin
SRC=src
BUILDDIR=build
BT2DB=$(BINDIR)/bt2db
BT2SCAN=$(BINDIR)/bt2scan
PROGRAMS=$(BT2DB) $(BT2SCAN)
LDFLAGS=-static-libgcc -static-libasan
#LDFLAGS=

all: $(PROGRAMS)

$(BUILDDIR)/buzz_opts.o: $(SRC)/buzz_opts.c $(INCLUDE)/buzz_opts.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/buzz_logging.o: $(SRC)/buzz_logging.c $(INCLUDE)/buzz_logging.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/bt2scan_api.o: $(SRC)/bt2scan_api.c $(INCLUDE)/bt2scan_api.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/bt2db_api.o: $(SRC)/bt2db_api.c $(INCLUDE)/bt2db_api.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BT2DB): $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2db.c 
	$(CC) $(CFLAGS) -o $(BT2DB) $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2db.c -static-libasan $(DB_LIBS) $(LDFLAGS)

$(BT2SCAN): $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2scan.c
	$(CC) $(CFLAGS) -o $(BT2SCAN) $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2scan.c -static-libasan $(BT_LIBS) $(LDFLAGS)


testargs: $(BUILDDIR)/buzz_logging.o tests/test_args.c $(BUILDDIR)/buzz_opts.o
	$(CC) $(CFLAGS) -o tests/testargs  $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o tests/test_args.c $(LDFLAGS)


.PHONY: db
db:
	sqlite3 locations.db < ddl/locations.ddl

.PHONY: clean

clean:
	rm -f $(BUILDDIR)/*.o $(BT2DB) $(BT2SCAN) tests/testargs
