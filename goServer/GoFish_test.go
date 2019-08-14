package main

import (
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"testing"
)

var goFish = &GoFish{NewServer(), NewBox("../private_config/box_jwt.json"), ""}

func TestGoFish_RunProcess(t *testing.T) {
	log.SetOutput(ioutil.Discard)
	goFish.RunProcess("ls")
	if len(goFish.server.GetProcesses()) < 1 {
		t.Fail()
	}
}

func TestGoFish_GetFishInfo(t *testing.T) {
	goFish.server.DB = CreateDatabase("FindingFish")
	r := &http.Request{}
	f := goFish.GetFishInfo(r)
	if f == nil {
		t.Fail()
	}
}

func TestGoFish_GetWorldPoints(t *testing.T) {
	os.Setenv("calib_config", "../calib_config/")
	r := &http.Request{}
	f := goFish.GetWorldPoints(r)
	if f == nil {
		t.Fail()
	}
}

func TestGoFish_GetFilenames(t *testing.T) {
	r := &http.Request{}
	f := goFish.GetFilenames(r)
	if f == nil {
		t.Fail()
	}
}

func TestGoFish_GetProcesses(t *testing.T) {
	r := &http.Request{}
	f := goFish.GetProcesses(r)
	if f == nil {
		t.Fail()
	}
}
