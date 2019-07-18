/* Temporary */
USE fishtest;

/* Create generic animal related tables */
CREATE TABLE family (fid SERIAL, name VARCHAR(50), PRIMARY KEY(fid));

CREATE TABLE genus(gid SERIAL, name VARCHAR(50), PRIMARY KEY(gid));

CREATE TABLE species(sid SERIAL, name VARCHAR(50), PRIMARY KEY(sid));

/* Create tables for information from the video processor */
CREATE TABLE events (eid SERIAL, PRIMARY KEY(eid));

CREATE TABLE activity(aid SERIAL, type VARCHAR(20), PRIMARY KEY(aid));

/* Create the more specific table for Fish */
CREATE TABLE fish (
 id SERIAL,
 fid BIGINT UNSIGNED NOT NULL,
 gid BIGINT UNSIGNED NOT NULL,
 sid BIGINT UNSIGNED NOT NULL,
 name VARCHAR(50), 
 PRIMARY KEY(id), 
 FOREIGN KEY(fid) REFERENCES family(fid),
 FOREIGN KEY(gid) REFERENCES genus(gid),
 FOREIGN KEY(sid) REFERENCES species(sid)
);
