/** 
 * @author Tomas Rigaux
 * @date May 8, 2019
 *
 * @file This is where the meat of the operations happen. All video related
 * handlers are defined here. Each is meant to handle different tasks relating
 * to how the video is displayed and is interacted with in browser. Basically,
 * this is where the video is modified so as to reduce the amount of work 
 * required by users to interact with it.
 */

/** The known framerate of the video. */
let FRAMERATE = 30;

/** Handles the modified video controls to keep the main canvas video the
 * custom scrubber bar. 
 */
class VideoHandler
{
    /** Default constructor for initializing all member variables. */
    constructor() 
    {
        this.video      = document.getElementById("selected");
        this.canvas     = document.getElementById("adjusted-video");
        this.ended      = false;
        this.newTime    = 0;

        if(this.video != null)
        {
            this.ended          = true;
            this.video.name     = "selected";
            this.video.maxFrame = Math.ceil(this.video.duration * FRAMERATE);

            var self = this;
            this.video.addEventListener("ended", function(){
                if($("#play-pause").hasClass("active"))
                {
                    $("#play-pause").removeClass("active");
                    $("#play-pause").children(".material-icons").html("&#xe037;");
                    self.pause();
                }
            });
        }

        this.draw();
    }

    /** Allows the video to be drawn to the canvas if the video is not ended or paused. */
    run()
    {
        if(this.video != null)
        {
            if(this.video.paused || this.video.ended)
                return;

            this.draw();
        }
    }

    /** Draws the canvas to the screen, and calculates whether or not the scrubber bar is keeping up. */
    draw()
    {
        if(this.canvas != null)
        {
            var context = this.canvas.getContext('2d');
            if(this.video != null)
            {
                context.drawImage(this.video, 0, 0, this.canvas.width, this.canvas.height);

                if(!this.video.paused)
                {
                    // Calculate the length of the combined video.
                    var time = this.video.maxFrame;
                    var scrubber = document.getElementById("event-time-bar");
                    
                    scrubber.max = time;
                    var ctx = scrubber.getContext("2d");
                    ctx.scale(10, 1);
                    scrubber.height = 40;

                    var newTime = parseFloat(scrubber.value / FRAMERATE);
                    if(scrubber.value <= Math.round(this.video.currentTime * FRAMERATE))
                        scrubber.value = Math.round(this.video.currentTime * FRAMERATE);
                    else this.adjustVideo(newTime - Number(this.video.currentTime));
                    document.getElementById("adjusted-time").innerHTML = Math.round(parseFloat(scrubber.value));
                }
            }
        }
    }

    /** Pauses the video and the canvas drawing. */
    pause()
    {
        if(this.video != null)
            this.video.pause();
    }

    /** Plays the video and adjusts the scrubber bar accordingly. */
    play()
    {
        if(this.video != null)
        {
            this.video.play();
            this.ended = false;

            var scrubber = document.getElementById("event-time-bar");
            scrubber.value = Number(this.video.currentTime) * FRAMERATE;
            document.getElementById("adjusted-time").innerHTML = Math.ceil(parseFloat(scrubber.value));
        }
    }

    /** Adjusts the video's current time by the difference converted from frames. 
     * @param {number} diff The difference in frames to adjust the video by.
     */
    adjustVideo(diff)
    {
        var self = this;
        setTimeout(function(){
            var time = new Number(self.video.currentTime + (diff / FRAMERATE));
            if(time instanceof Number) {self.video.currentTime = new Number(time);}
        }, 0);
        this.video.currentTime = Number(Math.max(0, Math.min(this.video.currentTime, this.video.duration)));
    }
}

/** Gets the JSON from the Go server fed to the page, decodes it, and extracts
 * all the event data from it. 
 */
class JsonHandler
{
    /** Creates the new object with the collected JSON data and tag.
     * @param {string} tag The video tag name.
     * @param {string} video The video to reference.
     */
    constructor(tag, video)
    {
        this.handle = tag != null || (tag != "" && tag != "{}") ? JSON.parse(atob(tag)) : null;
        this.tag = tag;
        this.video = video;
        this.events = []
        if(this.handle != null)
        {
            if(this.Event_QRCode() != null) this.frameOffset = this.Event_QRCode().frame;
            if(this.handle.DetectedEvents.length > 0)
                for(var i = 1; i <= this.handle.DetectedEvents.length; i++)
                    this.Event_Activity(i);
        }
    }

    /** Finds the QR code event and returns it as a formatted event. 
     * @return {JSON} The QR event as JSON.
     */
    Event_QRCode()
    {
        function QR(e){return e["Event_QRCode"];}
        if(this.handle != null)
            if(this.handle.DetectedEvents.length > 0)
                if(this.handle.DetectedEvents.find(QR) != null)
                    return this.handle.DetectedEvents.find(QR)["Event_QRCode"];
        return null;
    }

    /** Finds all activity events and returns them as an array of formatted
     * event objects.
     * @param {number} id The ID of the event to choose from the array.
     */
    Event_Activity(id)
    {
        if(this.handle != null)
        {
            function Activity(e){return e["Event_Activity_"+id];}
            var event = this.handle.DetectedEvents.find(Activity) != null ? this.handle.DetectedEvents.find(Activity)["Event_Activity_"+id] : null;
            if(event != null)
                this.events.push(new Event(event.frame_start / this.video.maxFrame, event.frame_end / this.video.maxFrame));
        }
    }
}

/** Handles the rendering of the extracted JSON events into the scrubber bar. */
class EventHandler
{
    /** Default constructor. Initializes all member variables. */
    constructor()
    {
        this.canvas = document.getElementById("event-time-bar");
        this.events = []
        this.event_index = 0;
    }

    /** A wrapper for the draw method to keep with naming convention. */
    run()
    {
        this.draw();
    }

    /** Defines how to draw a line on the scrubber bar canvas.
     * @param {number} start The starting frame of the line area.
     * @param {number} end The ending frame of the line area.
     * @param {string} colour The colour to draw the line.
     */
    drawLine(start, end, colour)
    {
        if(this.canvas != null)
        {
            var ctx = this.canvas.getContext('2d');

            ctx.beginPath(); 
            ctx.moveTo(start + 0.5, 20);
            ctx.lineTo(end + 0.5, 20);
            ctx.lineWidth = this.canvas.height;
            ctx.strokeStyle = colour;
            ctx.stroke();
        }
    }

    /** Draws all the events as colourful sections on the scrubber bar, as well
     * as draws the current position of the video as a white line on top.
     */
    draw()
    {
        if(this.canvas != null)
        {
            // Clear the canvas for redrawing.
            var ctx = this.canvas.getContext('2d');
            ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

            // Draw events.
            for(var i = 0; i < this.events.length; i++)
                this.drawLine((this.events[i].frame_start) * this.canvas.width, this.events[i].frame_end * this.canvas.width, this.events[i].colour);

            // Draw the event_index position in the video.
            var scrubber = document.getElementById("event-time-bar");
            var slider = (scrubber.value / scrubber.max) * this.canvas.width;
            this.drawLine(slider, slider+5, "#FFFFFF");

            /* 
            // FIXME: This works for moving the scrubber bar to the next event, but not the actual video.
            if(this.events[this.event_index] != null)
            {
                //console.log(this.events);
                if(this.events[this.event_index].frame_start * this.canvas.width <= slider && slider <= this.events[this.event_index].frame_end * this.canvas.width);
                else if (slider / this.canvas.width > this.events[this.event_index].frame_end)
                    if(this.events[this.event_index+1] != null && this.event_index + 1 < this.events.length)
                    {
                        scrubber.value = (this.events[this.event_index+1].frame_start * scrubber.max);
                        console.log("Skipping to next Event");
                        this.event_index = (this.event_index + 1) % this.events.length;
                    }
            }
            */
        }
    }

    /** Adds an event to the array of events.
     * @param {number} start The starting frame of the event.
     * @param {number} end The ending frame of the event.
     */
    addEvent(start, end)
    {
        this.events.push(new Event(start, end));
        this.draw();
    }
}

/** An event object which holds information on the adjusted start and
 * end times.
 */
class Event
{
    /** Constructs an event within a given start and end range.
     * @param[in] start The starting frame of the event.
     * @param[in] end The ending frame of the event.
     */
    constructor(start, end)
    {
        this.frame_start = start;
        this.frame_end = end;
        this.colour = "#ff4546";
    }

    /** Creates a random RGB colour for the event to be drawn as. */
    getRandomColour() 
    {
        var letters = '0123456789ABCDEF';
        var colour = '#';
        for (var i = 0; i < 6; i++) {
            colour += letters[Math.floor(Math.random() * 16)];
        }
        return colour;
    }
}

