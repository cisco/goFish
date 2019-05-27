package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"syscall"
	"time"
)

func main() {
	os.Setenv("PORT", "80")
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
	server.BuildHTMLTemplate("static/videos.html", HandleVideoHTML)

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
