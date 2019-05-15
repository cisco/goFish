package main

import (
	"fmt"
	"os/exec"
	"syscall"
	"time"
)

func main() {
	p := Processor{-1, ""}
	go p.StartProcessor("./FishFinder")
	time.Sleep(1 * time.Second)
	StartServer()
}

func StartServer() {
	// Create an handle the HTML for the page.
	BuildHTMLTemplate("static/videos.html", HandleVideoHTML)

	// Create and run the server.
	CreateServer(80)
}

type Processor struct {
	PID int
	CMD string
}

// StartProcessor : Starts up a background process.
func (p *Processor) StartProcessor(str string) {
	// Run the process
	cmd := exec.Command(str)
	cmd.Start()
	if cmd.Process != nil {
		fmt.Println(cmd.Process.Pid)
		p.PID = cmd.Process.Pid
		p.CMD = str
		// Start looping timer to check that the process is still running.
		p.CallbackTimer()
	} else {
		p.PID = -1
		time.Sleep(1 * time.Second)
		// Try again to start the process.
		p.StartProcessor(str)
	}
}

// CallbackTimer : Runs recursively to check if a process is still running. If not, it's starts it up again.
func (p *Processor) CallbackTimer() {
	_, err := syscall.Getpgid(p.PID)
	if err == nil {
		time.Sleep(1 * time.Second)
		p.CallbackTimer()
	} else {
		p.StartProcessor(p.CMD)
	}
}
