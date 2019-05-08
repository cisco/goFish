package main

import (
	"fmt"
	"html/template"
	"io/ioutil"
	"net/http"
	"strconv"
	"strings"
)

func main() {
	HandleHTMLTemplate("static/upload.html")
	// Create and run the server.
	CreateServer(80)
}

// Create the server.
func CreateServer(port uint16) {
	// Serve static assets (.html, .css, etc.).
	fs := http.FileServer(http.Dir("static"))
	http.Handle("/static/", http.StripPrefix("/static/", fs))
	fs = http.FileServer(http.Dir("static/scripts"))
	http.Handle("/scripts/", http.StripPrefix("/scripts/", fs))
	fs = http.FileServer(http.Dir("static/videos"))
	http.Handle("/videos/", http.StripPrefix("/videos/", fs))

	// Create server in port.
	http.ListenAndServe(":"+strconv.Itoa(int(port)), nil)
}

// Function to print to server when no page available.
func ServerPrint(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Server: This is a Go HTTP Server")
	fmt.Println("HTTP Request")
}

// Handles video files uploaded by users using forms on the server.
func UploadFile(w http.ResponseWriter, r *http.Request, formValue string) string {
	fmt.Println("File Uploaded.")
	var format = "mp4"

	// Parse the form, looking for files with max size of 10mb.
	r.ParseMultipartForm(10 << 20)

	file, handler, err := r.FormFile(formValue)

	if err != nil {
		// Print to commnd line.
		fmt.Println("Error while retrieving the file!")
		fmt.Println(err)
		return ""
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
		fmt.Println("File uploaded successfully!")
		return tempFile.Name()
	}
	fmt.Println("Incorrect file format!")
	return ""
}

func GetFileName(r *http.Request, formField string) string {
	r.ParseMultipartForm(10 << 20)
	var file, handler, err = r.FormFile(formField)
	if err != nil {
		fmt.Print("Erro selecting file: ")
		fmt.Println(err)
	} else if handler != nil {
		fmt.Println(file)
		return handler.Filename
	}
	return ""
}

// Creates the HTML required based on submitted files.
func HandleHTMLTemplate(file string) {
	var template = template.Must(template.ParseFiles(file))
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			template.Execute(w, nil)
			return
		}

		var leftVideoName, rightVideoName = GetFileName(r, "select-video-1"), GetFileName(r, "select-video-2")

		// If we're not selecting video, then upload the videos instead.
		if leftVideoName != "" && rightVideoName != "" {
			fmt.Println("Selecting " + leftVideoName)
			fmt.Println("Selecting " + rightVideoName)
		} else {
			leftVideoName = UploadFile(w, r, "upload-video-1")
			rightVideoName = UploadFile(w, r, "upload-video-2")
		}

		w.Header().Add("Content-Type", "text/html")

		// Execute our crafted HTML response and submit values to the page.
		template.Execute(w, struct {
			VideosLoaded  bool
			LeftVideoSrc  string
			RightVideoSrc string
		}{
			(leftVideoName != "") && (rightVideoName != ""),
			leftVideoName,
			rightVideoName,
		})
	})
}
