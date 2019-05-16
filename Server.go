package main

import (
	"html/template"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strings"
)

// Server : Server object to handle HTTP requests.
type Server struct {
	MUX *http.ServeMux
}

// NewServer : Constructs a new Server type.
func NewServer(args ...func(*Server)) *Server {
	server := &Server{
		MUX: http.NewServeMux(),
	}

	for _, f := range args {
		f(server)
	}

	// Serve static assets (.html, .css, etc.).
	fs := http.FileServer(http.Dir("static"))
	server.MUX.Handle("/static/", http.StripPrefix("/static/", fs))

	log.Println("=== SERVER STARTED ===")

	return server
}

// ServerHTTP : Wrapper for the Server MUX ServeHTTP method.
func (s *Server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	s.MUX.ServeHTTP(w, r)
}

// BuildHTMLTemplate : Creates the HTML required based on submitted files.
func (s *Server) BuildHTMLTemplate(file string, fn func(*http.Request) interface{}) {
	var tmpl = template.Must(template.ParseFiles(file))
	s.MUX.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			tmpl.Execute(w, nil)
			return
		}

		// Get the function which defines what we do to the page.
		retval := fn(r)

		// Execute our crafted HTML response and submit values to the page.
		tmpl.Execute(w, retval)
	})
}

// FileInfo : Structure to contain information about the precise formatting of files being added to the server.
type FileInfo struct {
	Name    string
	Format  string
	MaxSize int64
}

// UploadFiles : Handles video files uploaded to the server by users using forms in the browser.
func UploadFiles(r *http.Request, formValue string, saveLocation string, fileInfo FileInfo) {
	formData := r.MultipartForm

	files := formData.File["upload-files"]

	// Iterate through each file and create a copy on the server.
	for i := range files {
		file, err := files[i].Open()

		defer file.Close()
		if err != nil {
			log.Println(err)
			return
		}

		if _, err := os.Stat("static/" + saveLocation); os.IsNotExist(err) {
			os.Mkdir("static/"+saveLocation, 0777)
		}
		out, err := ioutil.TempFile("static/"+saveLocation, fileInfo.Name+"*"+fileInfo.Format)

		defer out.Close()
		if err != nil {
			log.Println("Error: Unable to create the file to be written. Please enure you have correct write access priviliges.")
			return
		}

		_, err = io.Copy(out, file)

		if err != nil {
			log.Print("Error: ")
			log.Println(err)
			return
		}

		log.Println("  File uploaded successfully.")
		log.Println("  > Filename: " + files[i].Filename)
	}
	log.Println(" Finished Upload.")
}

// GetFileName : If the file is valid, returns the name of the file.
func GetFileName(r *http.Request, formField string) string {
	r.ParseMultipartForm(10 << 20)
	var file, handler, err = r.FormFile(formField)
	if err != nil {
		log.Print("Error: Selecting file, ")
		log.Println(err)
	} else if handler != nil && file != nil {
		return handler.Filename
	}
	return ""
}

// BuildHTMLTemplate : Creates the HTML required based on submitted files.
func BuildHTMLTemplate(file string, fn func(*http.Request) interface{}) {
	var tmpl = template.Must(template.ParseFiles(file))
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			tmpl.Execute(w, nil)
			return
		}

		// Get the function which defines what we do to the page.
		retval := fn(r)

		// Execute our crafted HTML response and submit values to the page.
		tmpl.Execute(w, retval)
	})
}

// VideoInfo : Structure that holds information about a video, which is passed to HTML.
type VideoInfo struct {
	Name string
	Tag  string
	JSON string
}

// HandleVideoHTML : Returns the names of videos selected in browser. If no files were selected, then try to upload
// the files.
func HandleVideoHTML(r *http.Request) interface{} {
	var leftVideoName, rightVideoName = GetFileName(r, "select-video-1"), GetFileName(r, "select-video-2")

	// If we're not selecting video, then upload the videos instead.
	if leftVideoName == "" && rightVideoName == "" {
		log.Println(" Uploading files:")
		//t := time.Now()
		//UploadFiles(r, "upload-videos", "videos/"+t.Format("2006-01-02-030405"), FileInfo{"video-", ".mp4", 10 << 20})
		UploadFiles(r, "upload-videos", "videos/", FileInfo{"V_", ".mp4", 10 << 20})
	} else {
		log.Println(" Selecting files:")
		log.Println(" > Left Video  : " + leftVideoName)
		log.Println(" > Right Video : " + rightVideoName)
	}

	leftTag, rightTag := strings.TrimSuffix(strings.TrimPrefix(leftVideoName, "V_"), ".mp4"), strings.TrimSuffix(strings.TrimPrefix(rightVideoName, "V_"), ".mp4")
	file, err := ioutil.ReadFile("static/video-info/DE_" + leftTag + ".json")
	leftJSON, rightJSON := "", ""
	if err == nil {
		leftJSON = string(file)
	}
	file, err = ioutil.ReadFile("static/video-info/DE_" + rightTag + ".json")
	if err == nil {
		rightJSON = string(file)
	}

	return struct {
		VideosLoaded   bool
		LeftVideoInfo  VideoInfo
		RightVideoInfo VideoInfo
	}{
		(leftVideoName != "") && (rightVideoName != ""),
		VideoInfo{leftVideoName, leftTag, leftJSON},
		VideoInfo{rightVideoName, rightTag, rightJSON},
	}
}
