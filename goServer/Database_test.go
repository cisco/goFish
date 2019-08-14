package main

import (
	"testing"
)

var DB = NewDatabase(5000)

func TestDatabase_ConnectDB(t *testing.T) {
	DB.ConnectDB("", "root", "")
}

func TestDatabase_CreateDB(t *testing.T) {
	DB.CreateDB("test")
}

func TestDatabase_Execute(t *testing.T) {
	_, err := DB.Execute("USE test")
	if err != nil {
		t.Fail()
	}
	_, err = DB.Execute("CREATE TABLE IF NOT EXISTS test (test INT)")
	if err != nil {
		t.Fail()
	}
	_, err = DB.Execute("INSERT INTO test (test) VALUES (0)")
	if err != nil {
		t.Fail()
	}
}

func TestDatabase_Query(t *testing.T) {
	r, err := DB.Query("SELECT * FROM test")
	if err != nil {
		t.Fail()
	}
	if r != nil {
		defer r.Close()
	} else {
		t.Fail()
	}
}
