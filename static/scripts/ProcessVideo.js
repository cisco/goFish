let video_handler = {
    seekFrame: function(video, direction)
    {
        if(video != null)
        {
            var slider = document.getElementById(video.name + "-slider");
            if(slider != null)
            {
                video.frameOffset += direction;
                video.frameOffset = Math.max(0, Math.min(video.frameOffset, Math.ceil(video.duration * 30)));
                slider.value = video.frameOffset;
                video.currentTime = parseFloat(slider.value) / 30;
                video.maxFrame     = Math.ceil(video.duration * 30);
                video.adjustedFrameSize = video.maxFrame - video.frameOffset;
            }
        }
    },

    handleSlider: function(video)
    { 
        if(video != null)
        {
            var slider = document.getElementById(video.name + "-slider");
            if(slider != null)
            {
                slider.max = Math.ceil(video.duration * 30);
                video.frameOffset = parseFloat(slider.value);
                video.maxFrame     = Math.ceil(video.duration * 30);
                video.adjustedFrameSize = video.maxFrame - video.frameOffset;

                slider.oninput = function () {
                    video.frameOffset = parseFloat(this.value);
                    video.currentTime = this.value / 30;
                    video.adjustedFrameSize = video.maxFrame - video.frameOffset;
                }
            }
        }
    },

    // This is a little didactic and probably shouldn't remain as a permanent function.
    fillVideoInfo: function(video)
    {
        if(video != null)
        {
            var elem = document.getElementById(video.name + '-info');
            if(elem != null)
            {
                elem.getElementsByClassName("time")[0].innerHTML = Math.round(video.currentTime * 30);
                elem.getElementsByClassName("offset")[0].innerHTML = video.frameOffset
                elem.getElementsByClassName("frames")[0].innerHTML = Math.ceil(video.duration * 30);
                elem.getElementsByClassName("duration")[0].innerHTML = video.duration;
            }
        }
    },

    timerCallback: function()
    {
        this.fillVideoInfo(processor.leftVideo);
        this.fillVideoInfo(processor.rightVideo);
        let self = this;
        setTimeout(function(){self.timerCallback()}, 0);
    },

    adjustMainVideo: function()
    {
        var scrubber = document.getElementById("scrubber");
        if(scrubber != null)
        {
            scrubber.onmousedown = function() {
                processor.pause();
                start_val = scrubber.value;
            }
            
            scrubber.onmouseup = function() {
                processor.play();
            }
            
            scrubber.oninput = function(e) {
                processor.adjustVideo(parseFloat(this.value));
                document.getElementById("adjusted-time").innerHTML = parseFloat(this.value);
            }
        }
    }
}

let processor = {
    load: function() 
    {
        this.leftVideo              = document.getElementById("left");
        this.rightVideo             = document.getElementById("right");
        this.canvas                 = document.getElementById("adjusted-video");
        let self = this;
        if(this.leftVideo != null)
        {
            this.leftVideo.name         = "left";
            this.leftVideo.frameOffset  = 0;
            this.leftVideo.maxFrame     = Math.ceil(this.leftVideo.duration * 30);
            this.leftVideo.adjustedFrameSize = this.leftVideo.maxFrame - this.leftVideo.frameOffset;
            this.leftVideo.addEventListener("ended", function(){
                self.rightVideo.pause()
            });
        }
        if(this.rightVideo != null)
        {
            this.rightVideo.name        = "right";
            this.rightVideo.frameOffset = 0;
            this.rightVideo.maxFrame    = Math.ceil(this.rightVideo.duration * 30);
            this.rightVideo.adjustedFrameSize = this.rightVideo.maxFrame - this.rightVideo.frameOffset;
            this.rightVideo.addEventListener("ended", function(){
                self.leftVideo.pause();
            });
        }
    },

    timerCallback: function()
    {
        if((this.leftVideo.paused && this.rightVideo.paused) ||
        (this.leftVideo.ended && this.rightVideo.ended))
        return;

        this.draw();
        let self = this;
        this.timerHandle = setTimeout(function(){self.timerCallback()}, 0);
    },

    draw: function()
    {
        var context = this.canvas.getContext('2d');

        // TODO: Replace these with a function that will draw the exact pixels with minimum loss.
        context.drawImage(this.leftVideo, 0, 0, this.canvas.width/2, this.canvas.height);
        context.drawImage(this.rightVideo, this.canvas.width/2, 0, this.canvas.width/2, this.canvas.height);

        // Calculate the length of the combined video.
        var time = Math.min(this.leftVideo.adjustedFrameSize, this.rightVideo.adjustedFrameSize);
        var scrubber = document.getElementById("scrubber");
        
        scrubber.max = time;
        
        function adjustTime(video) { return (video.currentTime - (video.frameOffset / 30)) * 30; }
        scrubber.value = Math.max(adjustTime(this.leftVideo), adjustTime(this.rightVideo));
        document.getElementById("adjusted-time").innerHTML = parseFloat(scrubber.value);
    },

    pause : function()
    {
        this.leftVideo.pause();
        this.rightVideo.pause();
        clearTimeout(this.timerHandle);
    },

    play: function()
    {
        this.leftVideo.play();
        this.rightVideo.play();
        this.timerCallback();
    },

    sync: function()
    {
        this.pause();
        this.leftVideo.currentTime = this.leftVideo.frameOffset / 30;
        this.rightVideo.currentTime = this.rightVideo.frameOffset / 30;
        this.play();
    },

    setFrameOffset: function(L, R)
    {
        this.leftVideo.frameOffset = L;
        this.rightVideo.frameOffset = R;
    },

    adjustVideo: function(diff)
    {
        this.leftVideo.currentTime = (diff + this.leftVideo.frameOffset) / 30;
        this.rightVideo.currentTime = (diff + this.rightVideo.frameOffset) / 30;
    }
}

let json_handler = {
    load: function(tag)
    {
        this.handle =  JSON.parse(tag);
        this.tag = tag;
    },

    Event_QRCode: function()
    {
        if(this.handle != null)
            if(this.handle.DetectedEvents.length > 0)
                return this.handle.DetectedEvents[0]["Event_QRCode"];
        return null;
    },

    Event_Activity: function(id)
    {
        if(this.handle != null)
            if(this.handle.DetectedEvents[id] != null)
                return this.handle.DetectedEvents[id]["Event_Activity_"+id];

        return null;
    },

    timerCallback: function(video)
    {
        if(this.handle != null)
        {
            var activity = false;
            for(i = 1; i < json_handler.handle.DetectedEvents.length; i++)
            {
                var Event = json_handler.Event_Activity(i);
                var actOutput = document.getElementById("activity");
                if(Event.frame_start <= video.currentTime * 30 && video.currentTime * 30 <= Event.frame_end)
                {
                    activity = true;
                    actOutput.innerHTML = "Activity Happening";
                }
                if(!activity) actOutput.innerHTML = "None";
            }
        }
        let self = this;
        this.timerHandle = setTimeout(function(){self.timerCallback(video)}, 0);
    }
}

