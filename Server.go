package main

import (
	"fmt"
	"html/template"
	"io"
	"net/http"
	"os"
	"strconv"
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
	fs = http.FileServer(http.Dir("static/ videos/"))
	http.Handle("/videos/", http.StripPrefix("/videos/", fs))

	// Create server in port.
	http.ListenAndServe(":"+strconv.Itoa(int(port)), nil)
}

// Handles video files uploaded by users using forms on the server.
func UploadFile(w http.ResponseWriter, r *http.Request, formValue string) {
	fmt.Println("File Uploaded.")
	var format = "mp4"

	formData := r.MultipartForm

	files := formData.File["upload-videos"]

	for i, _ := range files {
		file, err := files[i].Open()
		defer file.Close()

		if err != nil {
			fmt.Println(err)
			return
		}

		out, err := os.Create("static/videos" + "video-*." + format)

		defer out.Close()

		if err != nil {
			fmt.Println("Unable to create the file to be written. Please enure you have correct write access priviliges.")
			return
		}

		_, err = io.Copy(out, file)

		if err != nil {
			fmt.Println(err)
			return
		}

		fmt.Println("Files uploaded successfully.")
		fmt.Println("Filename: " + files[i].Filename)

	}
}

// If the file is valid, returns the name of the file.
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
			UploadFile(w, r, "upload-videos")
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
