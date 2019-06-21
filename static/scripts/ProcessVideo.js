let FRAMERATE = 30;

class VideoHandler
{
    constructor() 
    {
        this.video  = document.getElementById("selected");
        this.canvas     = document.getElementById("adjusted-video");
        this.ended      = false;

        if(this.video != null)
        {
            this.ended          = true;
            this.video.name     = "selected";
            this.video.maxFrame = Math.ceil(this.video.duration * FRAMERATE);

            this.video.addEventListener("ended", function(){
                if($("#play-pause").hasClass("active"))
                {
                    $("#play-pause").removeClass("active");
                    $("#play-pause").children(".material-icons").html("&#xe037;");
                    pause();
                }
            });
        }

        this.draw();
    }

    run()
    {
        if(this.video != null)
        {
            if(this.video.paused || this.video.ended)
                return;

            this.draw();
        }
    }

    draw()
    {
        if(this.canvas != null)
        {
            var context = this.canvas.getContext('2d');
            if(this.video != null)
            {
                context.drawImage(this.video, 0, 0, this.canvas.width, this.canvas.height);

                // Calculate the length of the combined video.
                var time = this.video.maxFrame;
                var scrubber = document.getElementById("event-time-bar");
                
                scrubber.max = time;
                var ctx = scrubber.getContext("2d");
                ctx.scale(10, 1);
                scrubber.height = 40;

                scrubber.value = Math.round(this.video.currentTime * FRAMERATE);
                document.getElementById("adjusted-time").innerHTML = Math.round(parseFloat(scrubber.value));
            }
        }
    }

    pause()
    {
        if(this.video != null)
            this.video.pause();
    }

    play()
    {
        if(this.video != null)
        {
            this.video.play();
            this.ended = false;

            var scrubber = document.getElementById("event-time-bar");
            scrubber.value = this.video.currentTime * FRAMERATE;
            document.getElementById("adjusted-time").innerHTML = Math.ceil(parseFloat(scrubber.value));
        }
    }

    adjustVideo(diff)
    {
        this.video.currentTime += diff / FRAMERATE;
        this.video.currentTime = Math.max(0, Math.min(this.video.currentTime, this.video.duration));
    }
}

class JsonHandler
{
    constructor(tag, video)
    {
        this.handle = tag != null || tag != "" ? JSON.parse(tag) : null;
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

    Event_QRCode()
    {
        function QR(e){return e["Event_QRCode"];}
        if(this.handle != null)
            if(this.handle.DetectedEvents.length > 0)
                if(this.handle.DetectedEvents.find(QR) != null)
                    return this.handle.DetectedEvents.find(QR)["Event_QRCode"];
        return null;
    }

    Event_Activity(id)
    {
        if(this.handle != null)
        {
            function Activity(e){return e["Event_Activity_"+id];}
            var event = this.handle.DetectedEvents.find(Activity) != null ? this.handle.DetectedEvents.find(Activity)["Event_Activity_"+id] : null;
            if(event != null)
                this.events.push(new Event(((event.frame_start )/ (this.video.maxFrame)), ((event.frame_end) / (this.video.maxFrame))));
        }
    }
}

class EventHandler
{
    constructor()
    {
        this.canvas = document.getElementById("event-time-bar");
        this.events = []
        this.event_index = 0;
    }

    run()
    {
        this.draw();
    }

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

            if(this.events[this.event_index] != null)
            {
                console.log(this.events);
                if(this.events[this.event_index].frame_start * this.canvas.width <= slider && slider <= this.events[this.event_index].frame_end * this.canvas.width);
                else if (slider / this.canvas.width > this.events[this.event_index].frame_end)
                    if(this.events[this.event_index+1] != null && this.event_index + 1 < this.events.length)
                    {
                        scrubber.value = (this.events[this.event_index+1].frame_start * scrubber.max);
                        console.log("Skipping to next Event");
                        this.event_index++;
                    }
            }
        }
    }

    addEvent(start, end)
    {
        this.events.push(new Event(start, end));
        this.draw();
    }
}

class Event
{
    constructor(start, end)
    {
        this.frame_start = start;
        this.frame_end = end;
        this.colour = this.getRandomColour();
    }

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

