package main

func main() {
	// Create an handle the HTML for the page.
	BuildHTMLTemplate("static/videos.html", HandleVideoHTML)

	// Create and run the server.
	CreateServer(80)
}
