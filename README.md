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
- Interact with hardware at a low level.
- Learn to use the GPS flight control modules.
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
3. bt2gps: This is the internal API for interacting with the GPS hardware,
   Navigation Satellite Positioning NEO-6M: https://www.amazon.com/dp/B084MK8BS2?psc=1&ref=ppx_yo2ov_dt_b_product_details
4. This is the main program that puts it all together. It does the following:
    - Periodically scans for Bluetooth LE advertisements.
    - When found it gets the last know GPS coordinates.
    - Writes the information to an sqlite database.

## GPS

When inside the GPS coordinates are often not available. However, bluetooth
devices are often found indoors. To manage this case there is a backgound
thread that runs and gathers GPS coordinates every N seconds. The last known
coordinates are saved.  Thus when a new device is found inside we will
associate it with the last know location (likelt to be right outside the
building).

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

To build the programs simply run `make`.  This will create four programs:

- `./bin/bt2scan`: A program that will scan and print Bluetooth LE devices in the area
- `./bin/bt2db`: A program that can be used to add records to the database.
- `./bin/bt2gps`: A program that will print the current GPS coordinates.
- `./bin/bt2record`: The main program and goal of this project. It scans for BT devices and writes their GPS coordinates
to the database.

The first three programs are just internal tools. `bt2record` ties them
altogether in to the single functionality needed.  `bt2record` can be
run as a system service. There is an example systemd service file under
the etc directory. Setting up the service is left to the user.

## Usage

All of the programs have usage information baked into them. Simply run
then with the `--help` option to see usage details.

This distribution contains no tools for searching the data. The data
is written to an sqlite3 database. The `sqlite3` CLI can be used to inspect
it. The DDL of the database can be found under the `ddl/` directory.

That said, there is a way to disable location updates for specific devices. 
via the database. This is a desirable for the use case where you have
your own device that is always near the scanner (a watch for example).
The scanner will constantly find this device and write its location to
the database. This adds a lot of noice to your data. To disable a 
specific device from being tracked set the `update_location` field
in the `devices` table.  ex:

```
 $ sqlite3 locations.db
SQLite version 3.34.1 2021-01-20 14:10:07
Enter ".help" for usage hints.
sqlite> select * from devices;
1|2023-06-20 19:52:36|Forerunner 935|F9:44:AF:19:FE:4F|1
```

The above query shows my garmin watch. To prevent it from being tracked
run:

```
sqlite> update devices set update_location = 0 where id = 1;
sqlite> select * from devices;
1|2023-06-20 19:52:36|Forerunner 935|F9:44:AF:19:FE:4F|0
```

Now we see the `update_location` field is false and it will no longer be
tracked.


