package main

import (
	"net/http"
	"testing"
	"time"
)

var server = NewServer()

func TestServer_SetServeInfo(t *testing.T) {
	s := struct{ Test string }{"test"}
	server.SetServeInfo(s)

	r := &http.Request{}
	res := server.ServeInfo(r)

	if res == nil || (res.(map[string]interface{})["Test"]).(string) != "test" {
		t.Fail()
	}
}

func TestServer_(t *testing.T) {

}

func TestServer_AddProcess(t *testing.T) {
	server.AddProcess(&Process{"test", 0, "active", time.Now(), time.Now()})

}

func TestServer_GetProcesses(t *testing.T) {
	p := server.GetProcesses()
	if len(p) < 1 {
		t.Fail()
	}
	p[0].Status = "dead"
	time.Sleep(time.Second * 10)
	p = server.GetProcesses()
	if len(p) > 0 {
		t.Fail()
	}
}
