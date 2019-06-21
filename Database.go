package main

import (
	"database/sql"
	"log"

	_ "github.com/go-sql-driver/mysql"
)

// Database : A structure for holding relevant DB information.
type Database struct {
	db   *sql.DB
	port int
}

// NewDatabase : Constructor for Database object.
func NewDatabase(port int) *Database {
	db := &Database{nil, port}
	return db
}

// ConnectDB : Wrapper funtion for connecting to the database.
func (db_ptr *Database) ConnectDB(url string) {
	db, err := sql.Open("mysql", "root:findingfish@tcp("+url+")/")
	if err != nil {
		log.Println(err)
	} else {
		log.Println(" > Database connected succesfully!")
		db_ptr.db = db
	}
}

// CreateDB : Wrapper function for creating a a database. If the database already exists, then simply use it.
func (db_ptr *Database) CreateDB(name string) {
	if db_ptr.db != nil {
		_, err := db_ptr.Execute("CREATE DATABASE IF NOT EXISTS " + name)

		if err != nil {
			log.Println(err)
		} else {
			log.Println(" > Succesfully created database \"" + name + "\"")
		}

		_, err = db_ptr.Execute("USE " + name)
		if err != nil {
			log.Println(err)
		} else {
			log.Print(" > Selected database \"" + name + "\"")
		}
	}
}

// Execute : Wrapper function for executing SQL queries.
func (db_ptr *Database) Execute(query string, args ...interface{}) (sql.Result, error) {
	return db_ptr.db.Exec(query)
}

// Query : Wrapper function for querying SQL.
func (db_ptr *Database) Query(query string, args ...interface{}) (*sql.Rows, error) {
	return db_ptr.db.Query(query)
}
