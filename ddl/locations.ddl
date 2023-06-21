
CREATE TABLE IF NOT EXISTS devices (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    create_time         DATETIME DEFAULT CURRENT_TIMESTAMP,
    name                string,
    address             string UNIQUE,
    update_location     INTEGER default 1
);

CREATE TABLE IF NOT EXISTS locations (
    id                  INTEGER PRIMARY KEY AUTOINCREMENT,
    create_time         DATETIME DEFAULT CURRENT_TIMESTAMP,
    lat                 REAL,
    long                REAL,
    device_id           INTEGER,
    CONSTRAINT fk_devices
    FOREIGN KEY(device_id) REFERENCES devices(id)
);

PRAGMA foreign_keys=on;
