package main

import (
	"encoding/json"
	"fmt"
	"html/template"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"sort"
	"strings"
	"time"
)

// Server : Server object to handle HTTP requests.
type Server struct {
	MUX *http.ServeMux
	DB  *Database
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

// HandleHTTP : Wrapper for Server MUX Handle method.
func (s *Server) HandleHTTP(dir string, fn func(w http.ResponseWriter, r *http.Request)) {
	s.MUX.HandleFunc(dir, fn)
}

// BuildHTMLTemplate : Creates the HTML required based on submitted files.
func (s *Server) BuildHTMLTemplate(file string, dir string, fn func(*http.Request) interface{}) {
	var tmpl = template.Must(template.ParseFiles(file))
	s.MUX.HandleFunc(dir, func(w http.ResponseWriter, r *http.Request) {
		/*if r.Method != http.MethodPost {
			tmpl.Execute(w, nil)
			return
		}*/

		// Get the function which defines what we do to the page.
		retval := fn(r)

		// Execute our crafted HTML response and submit values to the page.
		tmpl.Execute(w, retval)
	})
}

// ServeInfo : Provides info from the database to the info panel.
func (s *Server) ServeInfo(r *http.Request) interface{} {

	var fileNames []string
	files, err := ioutil.ReadDir("./static/proc_videos/")

	if err == nil {
		for _, file := range files {
			if file.Name() != ".DS_Store" {
				fileNames = append(fileNames, file.Name())
			}
		}
	}

	return struct {
		PageInfo interface{}
		DBInfo   interface{}
		Files    []string
	}{
		HandleVideoHTML(r),
		GetDBInfo(s, r),
		fileNames,
	}
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

		var tag string
		if tag = "A"; i%2 == 1 {
			tag = "B"
		}
		out, err := os.Create("static/" + saveLocation + "/" + fileInfo.Name + "_" + tag + fileInfo.Format)

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

// VideoInfo : Structure that holds information about a video, which is passed to HTML.
type VideoInfo struct {
	Name string
	Tag  string
	JSON string
}

// HandleVideoHTML : Returns the names of videos selected in browser. If no files were selected, then try to upload
// the files.
func HandleVideoHTML(r *http.Request) interface{} {
	//err := r.ParseMultipartForm(10 << 20)
	if r.Method == "POST" {
		decoder := json.NewDecoder(r.Body)
		var d interface{}
		decoder.Decode(&d)

		//videoName := GetFileName(r, "select-video")
		videoName := d.(map[string]interface{})["file"].(string)

		if videoName == "" {
			return struct{ VideosLoaded bool }{false}
		}
		log.Println(" > Selecting video:" + videoName)

		tag := strings.TrimSuffix(videoName, ".mp4")
		file, err := ioutil.ReadFile("static/video-info/DE_" + tag + ".json")
		videoJSON := ""
		if err == nil {
			videoJSON = string(file)
		}

		return struct {
			VideosLoaded bool
			VideoInfo    VideoInfo
		}{
			videoName != "",
			VideoInfo{videoName, tag, videoJSON},
		}
	}
	return struct{ VideosLoaded bool }{false}
}

func HandleUpload(r *http.Request) interface{} {
	err := r.ParseMultipartForm(10 << 20)
	if err == nil {
		t := time.Now()
		UploadFiles(r, "upload-videos", "videos/", FileInfo{t.Format("2006-01-02-030405"), ".mp4", 10 << 20})
	}
	return struct{}{}
}

// HandleRulerHTML : Saves points gotten in browser to a YAML file to be read by an OpenCV program to triangulate the points.
func HandleRulerHTML(r *http.Request) interface{} {
	if r.Method == "POST" {
		decoder := json.NewDecoder(r.Body)
		var d interface{}
		decoder.Decode(&d)

		// Try to open the file. If it doesn't exist, create it, and write a default header.
		header := []byte("%YAML:1.0\n---")
		file, err := os.OpenFile("config/measure_points_"+strings.TrimPrefix(r.URL.Path, "/processing/")+".yaml", os.O_APPEND|os.O_WRONLY, 0600)
		if err != nil {
			log.Println(err)
			file, err = os.OpenFile("config/measure_points_"+strings.TrimPrefix(r.URL.Path, "/processing/")+".yaml", os.O_CREATE|os.O_WRONLY, 0600)
			if err != nil {
				log.Println(err)
			} else {
				file.Write(header)
			}
		}

		// Check to see if this file already contains data.
		temp, err := ioutil.ReadFile("config/measure_points_" + strings.TrimPrefix(r.URL.Path, "/processing/") + ".yaml")
		s := string(temp)
		name := "\nkeypoints:"
		if !strings.Contains(s, name) {
			file.Write([]byte(name))
		}

		// Build the formatted string of points to insert into the YAML file.
		outVector := ""
		for k, v := range d.(map[string]interface{}) {
			keys := make([]string, 0, len(v.(map[string]interface{})))
			for k := range v.(map[string]interface{}) {
				keys = append(keys, k)
			}
			sort.Strings(keys)
			m := v.(map[string]interface{})
			outVector += "\n\u0020\u0020\u0020" + k + ": [ "
			for i, k := range keys {
				outVector += fmt.Sprintf("%.3f", (m[k].(float64)))
				if i != len(keys)-1 {
					outVector += ", "
				}
			}
			outVector += " ]"
		}
		outVector = strings.TrimRight(outVector, ",")
		outVector += ""

		file.Write([]byte(outVector))

	}
	return struct{}{}
}

func GetDBInfo(s *Server, r *http.Request) interface{} {
	result, err := s.DB.Query("SELECT f.name, f.fid, g.name, g.gid, s.name, s.sid FROM fish, family AS f, genus AS g, species AS s WHERE fish.fid = f.fid AND fish.gid = g.gid AND fish.sid = s.sid;")
	if err != nil {
		log.Println(err)
		return struct{}{}
	}
	defer result.Close()

	type IdName struct {
		Id   int
		Name string
	}

	family := make([]IdName, 0)
	genera := make([]IdName, 0)
	species := make([]IdName, 0)
	for result.Next() {

		var f, g, s string
		var fid, gid, sid int
		err = result.Scan(&f, &fid, &g, &gid, &s, &sid)
		if err != nil {
			panic(err)
		}
		family = append(family, IdName{fid, f})
		genera = append(genera, IdName{gid, g})
		species = append(species, IdName{sid, s})
	}

	var fileNames []string
	files, err := ioutil.ReadDir("./static/proc_videos/")

	if err == nil {
		for _, file := range files {
			if file.Name() != ".DS_Store" {
				fileNames = append(fileNames, file.Name())
			}
		}
	}

	return struct {
		Family  []IdName
		Genera  []IdName
		Species []IdName
	}{
		family,
		genera,
		species,
	}
}
