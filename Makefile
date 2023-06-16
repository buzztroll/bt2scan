CC=gcc
INCLUDE=include
CFLAGS=-I$(INCLUDE) -g -Wall
BT_LIBS=-lbluetooth
DB_LIBS=-lsqlite3
BINDIR=bin
SRC=src
BUILDDIR=build
BT2DB=$(BINDIR)/bt2db
BT2SCAN=$(BINDIR)/bt2scan
PROGRAMS=$(BT2DB) $(BT2SCAN)

all: $(PROGRAMS)


$(BUILDDIR)/buzz_logging.o: $(SRC)/buzz_logging.c $(INCLUDE)/buzz_logging.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/bt2scan_api.o: $(SRC)/bt2scan_api.c $(INCLUDE)/bt2scan_api.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BUILDDIR)/bt2db_api.o: $(SRC)/bt2db_api.c $(INCLUDE)/bt2db_api.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BT2DB): $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/buzz_logging.o src/cmd/bt2db.c
	$(CC) $(CFLAGS) -o $(BT2DB) $(BUILDDIR)/bt2db_api.o $(BUILDDIR)/buzz_logging.o src/cmd/bt2db.c $(DB_LIBS)

$(BT2SCAN): $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/buzz_logging.o src/cmd/bt2scan.c
	$(CC) $(CFLAGS) -o $(BT2DB) $(BUILDDIR)/bt2scan_api.o $(BUILDDIR)/buzz_logging.o src/cmd/bt2scan.c $(BT_LIBS)

.PHONY: clean

clean:
	rm -f $(BUILDDIR)/*.o $(BT2DB) $(BT2SCAN)
