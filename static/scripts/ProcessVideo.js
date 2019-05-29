let FRAMERATE = 30;

class VideoSettingsHandler
{
    constructor(pcr)
    {
        this.pcr = pcr;
        this.handleSlider(pcr.leftVideo);
        this.handleSlider(pcr.rightVideo);
        this.adjustMainVideo();
        this.fillVideoInfo(pcr.leftVideo);
        this.fillVideoInfo(pcr.rightVideo);
    }

    seekFrameOffset(video, direction)
    {
        if(video != null)
        {
            var slider = document.getElementById(video.name + "-slider");
            if(slider != null)
            {
                video.frameOffset += direction;
                video.frameOffset = Math.max(0, Math.min(video.frameOffset, Math.ceil(video.duration * FRAMERATE)));
                slider.value = video.frameOffset;
                video.currentTime = parseFloat(slider.value) / FRAMERATE;
                video.maxFrame     = Math.ceil(video.duration * FRAMERATE);
                video.adjustedFrameSize = video.maxFrame - video.frameOffset;
                this.fillVideoInfo(video);
            }
        }
    }

    seekFrame(video, direction)
    {
        if(video != null)
        {
            video.currentTime += direction;
            video.currentTime = Math.max(0, video.currentTime);
            video.currentTime = Math.min(video.duration, video.currentTime);            
            this.fillVideoInfo(video);
        }
    }

    handleSlider(video)
    { 
        if(video != null)
        {
            var slider = document.getElementById(video.name + "-slider");
            if(slider != null)
            {
                slider.max = Math.ceil(video.duration * FRAMERATE);
                video.frameOffset = parseFloat(slider.value);
                video.maxFrame     = Math.ceil(video.duration * FRAMERATE);
                video.adjustedFrameSize = video.maxFrame - video.frameOffset;
                
                let self = this;
                slider.oninput = function () {
                    video.frameOffset = parseFloat(this.value);
                    video.currentTime = this.value / FRAMERATE;
                    video.adjustedFrameSize = video.maxFrame - video.frameOffset;
                    self.fillVideoInfo(video);
                }
            }
        }
    }

    fillVideoInfo(video)
    {
        if(video != null)
        {
            var elem = document.getElementById(video.name + '-info');
            if(elem != null)
            {
                elem.getElementsByClassName("time")[0].innerHTML = Math.round(video.currentTime * FRAMERATE);
                elem.getElementsByClassName("offset")[0].innerHTML = video.frameOffset
                elem.getElementsByClassName("frames")[0].innerHTML = Math.ceil(video.duration * FRAMERATE);
                elem.getElementsByClassName("duration")[0].innerHTML = video.duration;
            }
        }
    }

    run()
    {
        this.fillVideoInfo(this.pcr.leftVideo);
        this.fillVideoInfo(this.pcr.rightVideo);
       // let self = this;
       // this.timerHandle = setTimeout(function(){self.timerCallback()}, 0);
    }

    clearTimer()
    {
        clearTimeout(this.timerHandle);
    }

    adjustMainVideo()
    {
        var scrubber = document.getElementById("scrubber");
        if(scrubber != null)
        {
            scrubber.onmousedown = function() {
                this.pcr.pause();
                start_val = scrubber.value;
            }
            
            scrubber.onmouseup = function() {
                this.pcr.play();
            }
            
            scrubber.oninput = function(e) {
                this.pcr.adjustVideo(parseFloat(this.value));
                document.getElementById("adjusted-time").innerHTML = parseFloat(this.value);
            }
        }
    }
}

class Processor
{
    constructor() 
    {
        this.leftVideo              = document.getElementById("left");
        this.rightVideo             = document.getElementById("right");
        this.canvas                 = document.getElementById("adjusted-video");

        let self = this;
        if(this.leftVideo != null)
        {
            this.leftVideo.name         = "left";
            this.leftVideo.frameOffset  = 0;
            this.leftVideo.maxFrame     = Math.ceil(this.leftVideo.duration * FRAMERATE);
            this.leftVideo.adjustedFrameSize = this.leftVideo.maxFrame - this.leftVideo.frameOffset;

            this.leftVideo.addEventListener("ended", function(){
                self.rightVideo.pause()
            });
        }
        if(this.rightVideo != null)
        {
            this.rightVideo.name        = "right";
            this.rightVideo.frameOffset = 0;
            this.rightVideo.maxFrame    = Math.ceil(this.rightVideo.duration * FRAMERATE);
            this.rightVideo.adjustedFrameSize = this.rightVideo.maxFrame - this.rightVideo.frameOffset;

            this.rightVideo.addEventListener("ended", function(){
                self.leftVideo.pause();
            });
        }

        this.draw();
    }

    run()
    {
        if(this.leftVideo != null && this.rightVideo != null)
        {
            if((this.leftVideo.paused && this.rightVideo.paused) || (this.leftVideo.ended && this.rightVideo.ended))
                return;

            function CurrentFrame(video) { return Math.ceil(video.currentTime * FRAMERATE) - video.frameOffset; }

            var max_video = CurrentFrame(this.leftVideo) >= CurrentFrame(this.rightVideo) ? this.leftVideo : this.rightVideo;
            var min_video = max_video == this.leftVideo ? this.rightVideo : this.leftVideo;
            if( CurrentFrame(max_video) - CurrentFrame(min_video) > 1)
            {
                console.log("Out of sync! Syncing...");
                max_video.pause();
                setTimeout(function(){max_video.play();}, (CurrentFrame(max_video) - CurrentFrame(min_video)) / FRAMERATE); 
            }

            this.draw();
        }
    }

    draw()
    {
        if(this.canvas != null)
        {
            var context = this.canvas.getContext('2d');
            if(this.leftVideo != null && this.rightVideo != null)
            {
                // TODO: Replace these with a function that will draw the exact pixels with minimum loss.
                context.drawImage(this.leftVideo, 0, 0, this.canvas.width/2, this.canvas.height);
                context.drawImage(this.rightVideo, this.canvas.width/2, 0, this.canvas.width/2, this.canvas.height);

                // Calculate the length of the combined video.
                var time = Math.min(this.leftVideo.adjustedFrameSize, this.rightVideo.adjustedFrameSize);
                var scrubber = document.getElementById("event-time-bar");
                
                scrubber.max = time;
                var ctx = scrubber.getContext("2d");
                ctx.scale(10, 1);
                scrubber.height = 40;
                
                function adjustTime(video) { return ((video.currentTime * FRAMERATE) - (video.frameOffset)); }
                scrubber.value = Math.max(adjustTime(this.leftVideo), adjustTime(this.rightVideo));
                document.getElementById("adjusted-time").innerHTML = Math.ceil(parseFloat(scrubber.value));
            }
        }
    }

    pause()
    {
        if(this.leftVideo != null && this.rightVideo != null)
        {
            this.leftVideo.pause();
            this.rightVideo.pause();
            clearTimeout(this.timerHandle);
        }
    }

    play()
    {
        if(this.leftVideo != null && this.rightVideo != null)
        {
            if(this.leftVideo.ended || this.rightVideo.ended)
            {
                this.leftVideo.currentTime = this.leftVideo.frameOffset / FRAMERATE;
                this.rightVideo.currentTime = this.rightVideo.frameOffset / FRAMERATE;
            }
            this.leftVideo.play();
            this.rightVideo.play();

            function CurrentFrame(video) { return Math.ceil(video.currentTime * FRAMERATE) - video.frameOffset; }

            var max_video = CurrentFrame(this.leftVideo) >= CurrentFrame(this.rightVideo) ? this.leftVideo : this.rightVideo;
            var min_video = max_video === this.leftVideo ? this.rightVideo : this.leftVideo;
            if( CurrentFrame(max_video) > CurrentFrame(min_video)+1)
            {
                max_video.pause();
                max_video.currentTime = CurrentFrame(min_video) / FRAMERATE;
                setTimeout(function(){max_video.play();}, (CurrentFrame(max_video)-CurrentFrame(min_video))/FRAMERATE); 
            }

           // this.timerCallback();
        }
    }

    sync()
    {
        if(this.leftVideo != null && this.rightVideo != null)
        {
            this.pause();
            this.leftVideo.currentTime = this.leftVideo.frameOffset / FRAMERATE;
            this.rightVideo.currentTime = this.rightVideo.frameOffset / FRAMERATE;
            this.play();
        }
    }

    adjustVideo(diff)
    {
        this.leftVideo.currentTime = (diff + this.leftVideo.frameOffset) / FRAMERATE;
        this.rightVideo.currentTime = (diff + this.rightVideo.frameOffset) / FRAMERATE;
    }
}

class JsonHandler
{
    constructor(tag, video)
    {
        this.handle = tag != null ? JSON.parse(tag) : null;
        this.tag = tag;
        this.video = video;
        this.events = []
        if(this.handle != null)
            if(this.handle.DetectedEvents.length > 0)
                for(var i = 1; i < this.handle.DetectedEvents.length+1; i++)
                    this.Event_Activity(i);
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
                this.events.push(new Event(((event.frame_start - this.video.frameOffset )/ (this.video.maxFrame - this.video.frameOffset)), ((event.frame_end - this.video.frameOffset ) / (this.video.maxFrame - this.video.frameOffset))));
        }
    }
}

class EventHandler
{
    constructor()
    {
        this.canvas = document.getElementById("event-time-bar");
       /* this.canvas.addEventListener("mousedown", function(e){
            alert(e.pageX);
        });*/
        this.events = []
    }

    run()
    {
        this.draw();
    }

    clearTimer()
    {
        clearTimeout(this.timerHandle);
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
//            ctx.lineWidth = Math.abs(end - start);
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

            // Draw the current position in the video.
            var scrubber = document.getElementById("event-time-bar");
            var slider = (scrubber.value / scrubber.max) * this.canvas.width;
            this.drawLine(slider, slider+1, "#FFFFFF");
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

