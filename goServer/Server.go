package main

import (
	"html/template"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"reflect"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/fatih/structs"
	"github.com/mitchellh/mapstructure"
)

// Server : Server object to handle HTTP requests.
type Server struct {
	MUX       *http.ServeMux
	DB        *Database
	Box       *Box
	Info      interface{}
	Processes []*Process
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
		if fn != nil {
			// Get the function which defines what we do to the page.
			retval := fn(r)

			// Execute our crafted HTML response and submit values to the page.
			tmpl.Execute(w, retval)
		}
	})
}

// SetServeInfo : Sets the information that will be served.
func (s *Server) SetServeInfo(info interface{}) {
	s.Info = info
}

// ServeInfo : Serves the user defined info.
func (s *Server) ServeInfo(r *http.Request) interface{} {
	m := structs.Map(s.Info)
	for i, v := range m {
		if v != nil {
			if reflect.TypeOf(v).Kind() == reflect.Func {
				if v.(func(*http.Request) interface{}) != nil {
					m[i] = v.(func(*http.Request) interface{})(r)
				}
			}
		}
	}

	var result interface{}
	mapstructure.Decode(m, &result)

	return result
}

///////////////////////////////////////////////////////////////////////////////
// Files
///////////////////////////////////////////////////////////////////////////////

// FileFormat : Structure to contain information about the precise formatting
// of files being added to the server.
type FileFormat struct {
	Name      string
	Format    string
	MaxSize   int64
	AddSubtag bool
}

// UploadFiles : Handles video files uploaded to the server by users using
// forms in the browser.
func (s *Server) UploadFiles(r *http.Request, formValues []string, saveLocations []string, FileFormat FileFormat, boxFolderID string) {
	formData := r.MultipartForm

	for j, field := range formValues {
		files := formData.File[field]

		// Iterate through each file and create a temporary copy on the server, then send it to Box.
		var prevName string
		for i := range files {
			file, err := files[i].Open()

			if err != nil {
				log.Println(err)
				return
			}
			defer file.Close()

			if FileFormat.AddSubtag {
				FileFormat.Name = strings.Split(FileFormat.Name, "_")[0]
				var tag string
				if tag = "A"; i%2 == 1 {
					tag = "B"
				}
				var temp []string
				temp = append(temp, FileFormat.Name, tag)
				FileFormat.Name = strings.Join(temp, "_")
			}

			if i > 1 && !FileFormat.AddSubtag {
				FileFormat.Name = strings.Split(FileFormat.Name, "_")[0]
				var temp []string
				temp = append(temp, FileFormat.Name, strconv.Itoa(i))
				FileFormat.Name = strings.Join(temp, "_")
			}

			if prevName == FileFormat.Name {
				FileFormat.Name = strings.Split(FileFormat.Name, "_")[0]
				var temp []string
				temp = append(temp, FileFormat.Name, strconv.Itoa(i))
				FileFormat.Name = strings.Join(temp, "_")
			}

			saveLocation := saveLocations[j]
			if _, err := os.Stat(saveLocation); os.IsNotExist(err) {
				os.MkdirAll(saveLocation, 0744)
			}

			out, err := os.Create(saveLocation + FileFormat.Name + FileFormat.Format)
			if err != nil {
				log.Println(err)
				log.Println("Error: Unable to create the file to be written. Please enure you have correct write access priviliges.")
				return
			}
			defer out.Close()

			_, err = io.Copy(out, file)

			file, err = files[i].Open()
			if err != nil {
				log.Println(err)
				return
			}
			defer file.Close()

			contents, err := ioutil.ReadFile(out.Name())
			if err != nil {
				log.Println(err)
				return
			}

			if len(contents) > 0 {
				if boxFolderID != "" {
					// Upload temp file to box.
					fileObject, err := s.Box.UploadFile(contents, FileFormat.Name+FileFormat.Format, boxFolderID)
					log.Println(fileObject.Entries[0].Size)

					if err != nil {
						log.Println(err)
						log.Println("  * File not uploaded to Box!")
					}
				}

				if err != nil {
					log.Print("Error: ")
					log.Println(err)
					return
				}

				log.Println(" File uploaded successfully.")
				log.Println("  > New Filename: " + FileFormat.Name + FileFormat.Format)
			}
			prevName = FileFormat.Name
		}
		FileFormat.Name = strings.Split(FileFormat.Name, "_")[0]
	}
	if len(saveLocations) > 1 {
		os.Setenv("leftCameraDir", saveLocations[0])
		os.Setenv("rightCameraDir", saveLocations[1])
	}
	log.Println(" Finished Upload.")
}

///////////////////////////////////////////////////////////////////////////////
// Processes
///////////////////////////////////////////////////////////////////////////////

// Process : A programmatic representation of a process running on the server.
type Process struct {
	Name        string
	ID          int
	Status      string
	StartTime   time.Time
	ElapsedTime time.Time
}

// AddProcess : Adds a process to the server's list.
func (s *Server) AddProcess(p *Process) {
	s.Processes = append(s.Processes, p)
}

// GetProcesses : Returns a pointer to the list of processes running on the server.
func (s *Server) GetProcesses() []*Process {
	for _, v := range s.Processes {
		p, err := os.FindProcess(v.ID)
		if err != nil {
			log.Println(err)
		} else {
			err = p.Signal(syscall.Signal(0))
			if err != nil {
				// TODO: Mark for removal.
				v.Status = "dead"
			}
		}
	}

	return s.Processes
}
