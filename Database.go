package main

import (
	"database/sql"
	"log"

	_ "github.com/go-sql-driver/mysql"
)

type Database struct {
	db   *sql.DB
	port int
}

func NewDatabase(port int) *Database {
	db := &Database{nil, port}
	return db
}

func (db_ptr *Database) ConnectDB(url string) {
	db, err := sql.Open("mysql", "root@tcp/")
	log.Println(db.Ping())
	if err != nil {
		log.Println(err)
	} else {
		log.Println("Database created succesfully!")
		db_ptr.db = db
	}
}

func (db_ptr *Database) CreateDB(name string) {
	if db_ptr.db != nil {
		_, err := db_ptr.db.Exec("CREATE DATABASE " + name)

		if err != nil {
			log.Println(err)
		} else {
			log.Println("Succesfully created database \"" + name + "\"")
		}

		_, err = db_ptr.db.Exec("USE " + name)
		if err != nil {
			log.Println(err)
		} else {
			log.Print("Selected database \"" + name + "\"")
		}
	}

}
