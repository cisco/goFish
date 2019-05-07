package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"strconv"
	"strings"
)

func main() {
	// Process dynamic requests.
	http.HandleFunc("/", ServerPrint)

	// Process file uploads.
	http.HandleFunc("/upload", UploadFile)

	// Create and run the server.
	CreateServer(80)
}

// Create the server.
func CreateServer(port uint16) {
	// Serve static assets (.html, .css, etc.).
	fs := http.FileServer(http.Dir("static"))
	http.Handle("/static/", http.StripPrefix("/static/", fs))

	// Create server in port.
	http.ListenAndServe(":"+strconv.Itoa(int(port)), nil)
}

// Function to print to server when no page available.
func ServerPrint(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Server: This is a Go HTTP Server")
	fmt.Println("HTTP Request")
}

// Handles video files uploaded by users using forms on the server.
func UploadFile(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Server: File uploaded.\n")
	fmt.Println("File Uploaded.")
	var format = "mp4"

	// Parse the form, looking for files with max size of 10mb.
	r.ParseMultipartForm(10 << 20)

	file, handler, err := r.FormFile("upload-file")

	if err != nil {
		// Print to commnd line.
		fmt.Println("Error while retrieving the file!")
		fmt.Println(err)
		return
	}
	defer file.Close()

	// Check to make sure that the uploaded file is the correct format.
	if strings.Contains(strings.ToLower(handler.Filename), format) {
		fmt.Printf("Uploaded File: %+v\n", handler.Filename)
		fmt.Printf("Size: %+v\n", handler.Size)
		fmt.Printf("MIME Header: %+v\n", handler.Header)

		// Create a temporary file with a specific naming convention.
		tempFile, err := ioutil.TempFile("videos", "video-*."+format)
		if err != nil {
			fmt.Println(err)
		}
		defer tempFile.Close()

		fileBytes, err := ioutil.ReadAll(file)
		if err != nil {
			fmt.Println(err)
		}

		tempFile.Write(fileBytes)
		fmt.Fprintf(w, "File uploaded successfully!\n")
	} else {
		fmt.Fprintf(w, "Incorrect file format!\n")
		fmt.Println("Incorrect file format!")
	}
}
