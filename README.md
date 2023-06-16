# BuzzTroll Bluetooth Scanner (bt^2 scan)

This project uses Bluetooth to scan for advertising low-energy (LE)
devices in the area. When one is found its unique address and
name are recorded in an SQLite database along with the current GPS
coordinates.

The idea behind it is to carry a raspberry pi around and collect
data about the devices that you come into contact with. Is this
creepy? Probably. However there is no personal data attached. The
idea is just to see the increase in devices, and the types of devices
that I come into contact with overtime. I suspect as we move into
the future there will be more and more and this project will
provide data to study an interesting trend.

## Goals

I created this project just as a means of learning lower-level
systems programming on raspberry pi. I have the following goals:
- Interact with hardware.
- Learn the Bluetooth protocol.
- Have a fun C programming project.
- Port the entire project to Arduino to begin an embedded systems journey.
- Study data on how technology is saturating our daily lives more and more.

# Design

The project design is quite simple. I have three modules:
1. bt2scan: This is an internal API for interacting with Bluetooth. It
   currently uses bluez HCI implementation but I am hoping to remove this
   dependency and write my own for the learning experience.
2. bt2db: This is a thin wrapper around SQLite used to store the data.
3. bt2gps: This is the internal API for interacting with the GPS hardware (TBD).

## Code Layout

The code layout is very simple:
- src/ : source code for the APIs
- src/cmd : source code for any program
- include: the header files
- ddl: the database definition


## Building

Currently the project does not use autotools. This may be added someday
but because that is not one of my goals it is not likely.

To build the software you will need the following libraries:

- Bluetooth: `apt-get install bluez-source bluez-tools bluez-test-tools btscanner libbluetooth3 libbluetooth-dev`
- sqlite3: `apt-get install libsqlite3-dev`

You will also need a C development environment including gcc and make.

To build the programs simply run `make`.  This will create two programs:

- `./bin/bt2scan`: A program that will scan and print Bluetooth LE devices in the area
- `./bin/bt2db`: A program that can be used to add records to the database.

These two programs are just internal tools. As I work on the project there
will be a higher level program the encapsulates all the functionality.

