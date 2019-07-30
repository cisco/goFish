package main

import (
	"context"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"reflect"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/go-yaml/yaml"
)

func main() {
	args := os.Args[1:]
	if len(args) > 0 {
		os.Setenv("PORT", string(args[0]))
	} else {
		os.Setenv("PORT", "80")
	}

	// Network variables.
	os.Setenv("URL", "127.0.0.1:")
	os.Setenv("DB_PORT", "3306")

	// Box video folders.
	os.Setenv("mainFolder", "80559242908")
	os.Setenv("vidFolder", "80687476563")
	os.Setenv("procVidFolder", "80573476756")
	os.Setenv("vidInfoFolder", "82388040956")

	// Box calibration folders
	os.Setenv("calibFolder", "81405395430")
	os.Setenv("calibImgFolder", "81405876091")
	os.Setenv("calibResultFolder", "81406022555")

	goFish := &GoFish{NewServer(), NewBox("private_config/box_jwt.json"), ""}

	go goFish.ProcessAndUploadVideos("./static/videos/")
	go goFish.CalibrateCameras()

	goFish.StartServer()
}

///////////////////////////////////////////////////////////////////////////////
// GoFish
///////////////////////////////////////////////////////////////////////////////

// GoFish : The main object
type GoFish struct {
	server *Server
	box    *Box
	video  string
}

// StartServer : Starts up an HTTP server.
func (goFish *GoFish) StartServer() {
	// Bind interruption event.
	stop := make(chan os.Signal, 1)
	signal.Notify(stop, os.Interrupt)

	// Create an address for the correct port.
	addr := ":" + os.Getenv("PORT")
	if addr == ":" {
		addr = ":80"
	}

	//goFish.server.DB = CreateDatabase("fishtest")

	goFish.server.SetServeInfo(struct {
		PageInfo  func(*http.Request) interface{}
		PointInfo func(*http.Request) interface{}
		Files     func(*http.Request) interface{}
		Processes func(*http.Request) interface{}
	}{
		goFish.HandleVideoHTML,
		goFish.GetWorldPoints,
		goFish.GetFilenames,
		goFish.GetProcesses,
	})

	goFish.server.BuildHTMLTemplate("static/videos.html", "/", goFish.server.ServeInfo)
	goFish.server.BuildHTMLTemplate("static/videos.html", "/upload/", goFish.HandleVideoUpload)
	goFish.server.BuildHTMLTemplate("static/videos.html", "/calibrate/", goFish.HandleCalibrateUpload)
	goFish.server.BuildHTMLTemplate("static/videos.html", "/processing/", goFish.HandleRulerHTML)
	goFish.server.Box = goFish.box

	/*// This is for clearing out folders. It should be removed at some point.
	folder, _ := goFish.box.GetFolderItems(os.Getenv("vidInfoFolder"), 10, 0)
	log.Println(folder)

	for _, v := range folder.Entries {
		goFish.box.DeleteFile(v.ID, v.Etag)
	}
	*/

	handler := &http.Server{Addr: addr, Handler: goFish.server}

	go func() {
		if err := handler.ListenAndServe(); err != nil {
			log.Fatal(err)
		}
	}()

	// Check if interruption event happened
	<-stop
	log.Println("\r=> Gracefully shutting down the server...")

	context, cancel := context.WithTimeout(context.Background(), 5*time.Second)

	handler.Shutdown(context)
	cancel()

	log.Println("=== Server has Shutdown ===")
	os.Exit(1)
}

// ProcessAndUploadVideos : Handles processing videos and uploading the results
// to Box, then cleaning up the server-side files.
func (goFish *GoFish) ProcessAndUploadVideos(args ...string) {
	if len(args) > 0 {
		for {
			time.Sleep(1 * time.Second)
			if args[0] != "" {
				empty, size, err := IsDirEmpty(args[0])
				if err == nil {
					if !empty && size > 1 && (size%2) == 0 {
						goFish.RunProcess("./FishFinder")
					} else {
						empty, size, err = IsDirEmpty("./static/proc_videos")
						if !empty {
							dir, err := os.Open("./static/proc_videos")
							if err != nil {
								log.Println(err)
							}
							defer dir.Close()

							files, err := dir.Readdir(5)
							if len(files) > 0 {
								for _, file := range files {
									if file.Name() != ".DS_Store" {
										goFish.box.UploadFile("./static/proc_videos/"+file.Name(), file.Name(), os.Getenv("procVidFolder"))
										os.Remove("./static/proc_videos/" + file.Name())
										goFish.box.UploadFile("./static/video-info/DE_"+strings.TrimSuffix(file.Name(), ".mp4")+".json", "DE_"+strings.TrimSuffix(file.Name(), ".mp4")+".json", os.Getenv("vidInfoFolder"))
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

// CalibrateCameras : Calibrates files from specified directories.
func (goFish *GoFish) CalibrateCameras() {
	for {
		time.Sleep(1 * time.Second)
		empty, _, err := IsDirEmpty(os.Getenv("leftCameraDir"))
		if err == nil {
			empty, _, err = IsDirEmpty(os.Getenv("rightCameraDir"))
			if err == nil {
				if !empty {
					goFish.RunProcess("./Calibrate", os.Getenv("leftCameraDir"), os.Getenv("rightCameraDir"))
					err = os.RemoveAll(os.Getenv("leftCameraDir"))
					if err != nil {
						log.Println(err)
					}
					err = os.RemoveAll(os.Getenv("rightCameraDir"))
					if err != nil {
						log.Println(err)
					}
					os.Setenv("leftCameraDir", "")
					os.Setenv("rightCameraDir", "")
				}
			}
		}
	}
}

// RunProcess : Starts up a background process.
func (goFish *GoFish) RunProcess(instr string, args ...string) {
	// Run the process
	cmd := exec.Command(instr, args...)
	cmd.Start()
	defer cmd.Wait()

	if cmd.Process != nil {
		log.Printf("=== Started process %s with PID: %d ===\n", strings.TrimPrefix(instr, "./"), cmd.Process.Pid)
		goFish.server.AddProcess(&Process{strings.TrimPrefix(instr, "./"), cmd.Process.Pid, "active", time.Now(), time.Time{}})
	}
}

///////////////////////////////////////////////////////////////////////////////
// Client-side functions.
///////////////////////////////////////////////////////////////////////////////

// HandleVideoHTML : Returns the names of videos selected in browser. If no
// files were selected, then try to upload the files.
func (goFish *GoFish) HandleVideoHTML(r *http.Request) interface{} {
	// VideoInfo : Structure that holds information about a video, which is passed
	// to HTML.
	type VideoInfo struct {
		Name string
		Tag  string
		JSON string
	}
	if r.Method == "POST" || goFish.video != "" {
		decoder := json.NewDecoder(r.Body)
		var d interface{}
		decoder.Decode(&d)

		var videoName string
		if d != nil {
			if reflect.TypeOf(d).Kind() == reflect.Map {
				videoName = d.(map[string]interface{})["file"].(string)
			} else {
				videoName = goFish.video
			}
		}

		if videoName == "" && goFish.video == "" {
			return struct{ VideosLoaded bool }{false}
		} else if videoName == "" {
			videoName = goFish.video
		}

		if videoName != goFish.video {
			log.Println(" > Selecting video:" + videoName)

			// If file isn't already on the server, get it from box.
			_, err := os.Open("static/temp/" + videoName)
			if err != nil {
				items, err := goFish.box.GetFolderItems(os.Getenv("procVidFolder"), 1000, 0)
				var fileID string
				for _, v := range items.Entries {
					if v.Name == videoName {
						fileID = v.ID
						break
					}
				}

				err = goFish.box.DownloadFile(fileID, "static/temp/")
				if err != nil {
					log.Println(err)
					return struct{ VideosLoaded bool }{false}
				}

				_, err = os.Open("static/video-info/" + "DE_" + strings.TrimSuffix(videoName, ".mp4") + ".json")
				if err != nil {
					items, err = goFish.box.GetFolderItems(os.Getenv("vidInfoFolder"), 1000, 0)
					var infoID string
					for _, v := range items.Entries {
						if v.Name == "DE_"+strings.TrimSuffix(videoName, ".mp4")+".json" {
							infoID = v.ID
							break
						}
					}

					err = goFish.box.DownloadFile(infoID, "static/video-info/")
					if err != nil {
						log.Println(err)
						return struct{ VideosLoaded bool }{false}
					}
				}
			}
		}

		tag := strings.TrimSuffix(videoName, ".mp4")
		file, err := ioutil.ReadFile("static/video-info/DE_" + tag + ".json")
		videoJSON := ""
		if err == nil {
			videoJSON = string(file)
		}

		if videoName != goFish.video {
			goFish.video = videoName
		}

		return struct {
			VideosLoaded bool
			VideoInfo    VideoInfo
		}{
			goFish.video != "",
			VideoInfo{videoName, tag, base64.URLEncoding.EncodeToString([]byte(videoJSON))},
		}

	}

	return struct{ VideosLoaded bool }{false}
}

// HandleVideoUpload : Handles uploading video files.
func (goFish *GoFish) HandleVideoUpload(r *http.Request) interface{} {
	err := r.ParseMultipartForm(10 << 20)
	if err == nil {
		t := time.Now()
		formFields, saveLocations := make([]string, 1), make([]string, 1)
		formFields[0] = "upload-files"
		saveLocations[0] = "static/videos/"
		goFish.server.UploadFiles(r, formFields, saveLocations, FileFormat{t.Format("2006-01-02-030405"), ".mp4", 10 << 20, true}, os.Getenv("vidFolder"))
	}
	return struct{}{}
}

// HandleCalibrateUpload : Handles uploading calibration image files.
func (goFish *GoFish) HandleCalibrateUpload(r *http.Request) interface{} {
	err := r.ParseMultipartForm(10 << 20)
	if err == nil {
		t := time.Now()
		formFields, saveLocations := make([]string, 2), make([]string, 2)
		formFields[0] = "left-camera-files"
		formFields[1] = "right-camera-files"
		saveLocations[0] = "static/calibrate/" + r.FormValue("left-camera") + "/"
		saveLocations[1] = "static/calibrate/" + r.FormValue("right-camera") + "/"
		goFish.server.UploadFiles(r, formFields, saveLocations,
			FileFormat{t.Format("2006-01-02-030405"),
				".jpg",
				10 << 20, false},
			"")
	}
	return struct{}{}
}

// HandleRulerHTML : Saves points gotten in browser to a YAML file to be read
// by an OpenCV program to triangulate the points.
func (goFish *GoFish) HandleRulerHTML(r *http.Request) interface{} {
	if r.Method == "POST" {
		decoder := json.NewDecoder(r.Body)
		var d interface{}
		decoder.Decode(&d)

		// Try to open the file. If it doesn't exist, create it, and write a default header.
		header := []byte("%YAML:1.0\n---")
		file, err := os.OpenFile(
			"calib_config/measure_points.yaml",
			os.O_APPEND|os.O_WRONLY, 0600)
		if err != nil {
			log.Println(err)
			file, err = os.OpenFile(
				"calib_config/measure_points.yaml",
				os.O_CREATE|os.O_WRONLY, 0600)
			if err != nil {
				log.Println(err)
			} else {
				file.Write(header)
			}
		}

		// Build the formatted string of points to insert into the YAML file.
		outVector := ""
		for k, v := range d.(map[string]interface{}) {
			name := "\n" + k + ":"
			outVector += name
			for k, v := range v.(map[string]interface{}) {
				keys := make([]string, 0, len(v.(map[string]interface{})))
				if v != nil {
					for k := range v.(map[string]interface{}) {
						keys = append(keys, k)
					}
					sort.Strings(keys)
					m := v.(map[string]interface{})
					outVector += "\n\u0020\u0020\u0020" + k + ": [ "
					for i, k := range keys {
						outVector += fmt.Sprintf("%.6f", (m[k].(float64)))
						if i != len(keys)-1 {
							outVector += ", "
						}
					}
					outVector += "]\u0020\u0020\u0020"
				}
			}
		}
		outVector = strings.TrimRight(outVector, ",")
		outVector += ""

		file.WriteAt([]byte(outVector), 13)

		// FIXME: This is BAD. Need to find a way to wrap this in C and GO
		// to be called directly in Go, rather than a hacky c++ method.
		goFish.RunProcess("./FishFinder", "TRIANGULATE")

	}
	return struct {
		PointInfo func(r *http.Request) interface{}
	}{goFish.GetWorldPoints}
}

// GetFishInfo : Gets all the info required to fill out the info form for
// identifying animals.
func (goFish *GoFish) GetFishInfo(r *http.Request) interface{} {
	if goFish.server.DB.db != nil {

		result, err := goFish.server.DB.Query("SELECT f.name, f.fid, g.name, g.gid, s.name, s.sid FROM fish, family AS f, genus AS g, species AS s WHERE fish.fid = f.fid AND fish.gid = g.gid AND fish.sid = s.sid;")
		if err != nil {
			log.Println(err)
			return struct{}{}
		}
		defer result.Close()

		// IDName : Struct with a combo of ID and Name.
		type IDName struct {
			ID   int
			Name string
		}

		family := make([]IDName, 0)
		genera := make([]IDName, 0)
		species := make([]IDName, 0)
		for result.Next() {

			var f, g, s string
			var fid, gid, sid int
			err = result.Scan(&f, &fid, &g, &gid, &s, &sid)
			if err != nil {
				panic(err)
			}
			family = append(family, IDName{fid, f})
			genera = append(genera, IDName{gid, g})
			species = append(species, IDName{sid, s})
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
			Family  []IDName
			Genera  []IDName
			Species []IDName
		}{
			family,
			genera,
			species,
		}
	}
	return nil
}

// GetWorldPoints : Retrieves world points and sends them to the client to be
//  measured.
func (goFish *GoFish) GetWorldPoints(r *http.Request) interface{} {
	file, err := ioutil.ReadFile("calib_config/ObjectPoints.yaml")
	if err != nil {
		return nil
	}

	var js map[string][][]float64
	yaml.Unmarshal(file[14:], &js)

	pointArr := js["object_points"]
	return struct{ Points [][]float64 }{pointArr}
}

// GetFilenames : Gets all processed video files and returns them as a list.
func (goFish *GoFish) GetFilenames(r *http.Request) interface{} {
	items, err := goFish.box.GetFolderItems(os.Getenv("procVidFolder"), 1000, 0)

	if err != nil {
		log.Println(err)
	}

	var fileNames []string
	for _, v := range items.Entries {
		fileNames = append(fileNames, v.Name)
	}

	return struct{ Names []string }{fileNames}
}

// GetProcesses : Gets all processed video files and returns them as a list.
func (goFish *GoFish) GetProcesses(r *http.Request) interface{} {
	return struct{ P []*Process }{goFish.server.GetProcesses()}
}

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

// CreateDatabase : Handles the creation of and/or connection to a database.
func CreateDatabase(name string) *Database {
	addr := os.Getenv("DB_PORT")
	port, _ := strconv.Atoi(addr)
	db := NewDatabase(port)

	db.ConnectDB(os.Getenv("URL")+addr, "root", "findingfish")
	db.CreateDB(name)
	return db
}

// IsDirEmpty : Checks to see if there are any files in a directory.
func IsDirEmpty(name string) (bool, int, error) {
	dir, err := os.Open(name)
	if err != nil {
		return false, 0, err
	}
	defer dir.Close()

	files, err := dir.Readdir(3)

	if err == io.EOF || (len(files) == 1 && files[0].Name() == ".DS_Store") {
		return true, 0, nil
	}

	if files[0].Name() == ".DS_Store" {
		return false, len(files) - 1, err
	}
	return false, len(files), err

}
