package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"strconv"
	"syscall"
	"time"
)

func main() {
	args := os.Args[1:]
	if len(args) > 0 {
		os.Setenv("PORT", string(args[0]))
	} else {
		os.Setenv("PORT", "80")
	}
	os.Setenv("URL", "127.0.0.1:")
	os.Setenv("DB_PORT", "3306")
	//go RunProcess("./FishFinder")
	StartServer()
}

// StartServer : Starts up an HTTP server.
func StartServer() {
	// Bind interruption event.
	stop := make(chan os.Signal, 1)
	signal.Notify(stop, os.Interrupt)

	// Create an address for the correct port.
	addr := ":" + os.Getenv("PORT")
	if addr == ":" {
		addr = ":80"
	}

	// Create the new server.
	server := NewServer()
	server.DB = CreateDatabase()
	server.BuildHTMLTemplate("static/videos.html", "/", server.ServeInfo)
	server.BuildHTMLTemplate("static/videos.html", "/upload/", HandleUpload)
	server.BuildHTMLTemplate("static/videos.html", "/processing/", HandleRulerHTML)

	boxSDK := NewBoxSDK("database/211850911_ojaojsfr_config.json")
	//boxSDK.UploadFile("./GoFish.go", "Test8000.go", 0)
	boxSDK.DownloadFile(482462232448)
	//boxSDK.GetFolderItems(0, 10, 0)

	handler := &http.Server{Addr: addr, Handler: server}

	go func() {
		if err := handler.ListenAndServe(); err != nil {
			log.Fatal(err)
		}
	}()

	// Check if interruption event happened
	<-stop
	log.Println("=> Gracefully shutting down the server...")

	context, cancel := context.WithTimeout(context.Background(), 5*time.Second)

	handler.Shutdown(context)
	cancel()

	log.Println("=== Server has Shutdown ===")
	os.Exit(1)
}

// RunProcess : Starts up a background process.
func RunProcess(instr string) {
	// RunProcess the process
	cmd := exec.Command(instr)
	cmd.Start()
	if cmd.Process != nil {
		log.Printf("=== Started process with PID: %d ===\n", cmd.Process.Pid)

		// Start looping timer to check that the process is still runProcessning.
		for {
			time.Sleep(1 * time.Second)
			_, err := syscall.Getpgid(cmd.Process.Pid)
			if err != nil {
				log.Println("!!! Process died! Starting again...")
				RunProcess(instr)
			}
		}
	} else {
		time.Sleep(1 * time.Second)
		// Try again to start the process.
		RunProcess(instr)
	}
}

// CreateDatabase : Handles the creation of and/or connection to a database.
func CreateDatabase() *Database {
	addr := os.Getenv("DB_PORT")
	port, _ := strconv.Atoi(addr)
	db := NewDatabase(port)

	db.ConnectDB(os.Getenv("URL") + addr)
	db.CreateDB("fishTest")
	return db
}
