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
BT2GPS=$(BINDIR)/bt2gps
BT2RECORD=$(BINDIR)/bt2record
PROGRAMS=$(BT2DB) $(BT2SCAN) $(BT2GPS) $(BT2RECORD)
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

$(BUILDDIR)/bt2gps_api.o: $(SRC)/bt2gps_api.c $(INCLUDE)/bt2gps_api.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BT2DB): $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2db.c 
	$(CC) $(CFLAGS) -o $(BT2DB) $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2db.c -static-libasan $(DB_LIBS) $(LDFLAGS)

$(BT2SCAN): $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2scan.c
	$(CC) $(CFLAGS) -o $(BT2SCAN) $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o src/cmd/bt2scan.c -static-libasan $(BT_LIBS) $(LDFLAGS)

$(BT2GPS): $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o $(BUILDDIR)/bt2gps_api.o src/cmd/bt2gps.c
	$(CC) $(CFLAGS) -o $(BT2GPS) $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o $(BUILDDIR)/bt2gps_api.o  src/cmd/bt2gps.c -static-libasan $(LDFLAGS)

$(BT2RECORD): $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o $(BUILDDIR)/bt2gps_api.o $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/bt2scan_api.o src/cmd/bt2record.c
	$(CC) $(CFLAGS) -o $(BT2RECORD) $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o $(BUILDDIR)/bt2gps_api.o $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/bt2db_api.o  src/cmd/bt2record.c -static-libasan $(LDFLAGS) $(DB_LIBS) $(BT_LIBS)

test_args: $(BUILDDIR)/buzz_logging.o tests/test_args.c $(BUILDDIR)/buzz_opts.o
	$(CC) $(CFLAGS) -o tests/test-args  $(BUILDDIR)/buzz_logging.o $(BUILDDIR)/buzz_opts.o tests/test_args.c $(LDFLAGS) -lcunit

install: $(BT2RECORD) db
	cp bin/bt2record /usr/local/bin/bt2record

.PHONY: db
db:
	sqlite3 locations.db < ddl/locations.ddl

.PHONY: clean

clean:
	rm -f $(BUILDDIR)/*.o $(BT2DB) $(BT2SCAN) tests/testargs $(BT2RECORD) $(BT2GPS)
