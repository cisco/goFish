package main

import (
	"reflect"
	"testing"
)

var box = NewBox("../private_config/box_jwt.json")
var ID string

func TestBox_UploadFile(t *testing.T) {
	pc, err := box.UploadFile("Box_test.go", "test.txt", "0")
	if pc != nil && err == nil {
		ID = pc.Entries[0].ID
	} else {
		t.Fail()
	}
}

func TestBox_GetFileInfo(t *testing.T) {
	fo, err := box.GetFileInfo(ID)
	if err != nil && !reflect.DeepEqual(fo, &FolderObject{}) {
		t.Fail()
	}
}

func TestBox_GetEmbedLink(t *testing.T) {
	ef, err := box.GetEmbedLink(ID)
	if err != nil && !reflect.DeepEqual(ef, &EmbeddedFile{}) {
		t.Fail()
	}
}

func TestBox_DownloadFile(t *testing.T) {
	err := box.DownloadFile(ID, "../static/temp/")
	if err != nil {
		t.Fail()
	}
}

func TestBox_DeleteFile(t *testing.T) {
	err := box.DeleteFile(ID, "0")
	if err != nil {
		t.Fail()
	}
}

func TestBox_CreateFolder(t *testing.T) {
	fo, err := box.CreateFolder("Test", "0")
	if err != nil && !reflect.DeepEqual(fo, &FolderObject{}) {
		t.Fail()
	} else {
		ID = fo.ID
	}
}

func TestBox_GetFolderItems(t *testing.T) {
	ic, err := box.GetFolderItems("0", 10, 0)
	if err != nil && !reflect.DeepEqual(ic, &ItemCollection{}) {
		t.Fail()
	}
}

func TestBox_DeleteFolder(t *testing.T) {
	err := box.DeleteFolder(ID)
	if err != nil {
		t.Fail()
	}
}
