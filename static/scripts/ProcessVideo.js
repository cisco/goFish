let video_handler = {
    seekFrame: function(video, direction)
    {
        var slider = document.getElementById(video.name + "-slider");
        var output = document.getElementById(video.name + "-offset");
        video.frameOffset += direction;
        video.frameOffset = Math.max(0, Math.min(video.frameOffset, Math.ceil(video.duration * 30)));
        slider.value = video.frameOffset;
        output.innerHTML = video.frameOffset;
        video.maxFrame     = Math.ceil(video.duration * 30);
        video.adjustedFrameSize = video.maxFrame - video.frameOffset;
        console.log(video.maxFrame);
    },

    handleSlider: function(video)
    { 
        var slider = document.getElementById(video.name + "-slider");
        var output = document.getElementById(video.name + "-offset");
        slider.max = Math.ceil(video.duration * 30);
        output.innerHTML = video.frameOffset = parseFloat(slider.value);
        video.maxFrame     = Math.ceil(video.duration * 30);
        video.adjustedFrameSize = video.maxFrame - video.frameOffset;

        slider.oninput = function () {
            output.innerHTML = video.frameOffset = parseFloat(this.value);
            video.currentTime = this.value / 30;
        }
    },

    fillVideoInfo: function(video)
    {
        var elem = document.getElementById(video.name + '-video');
        elem.getElementsByClassName("time")[0].innerHTML = video.currentTime;
        elem.getElementsByClassName("offset")[0].innerHTML = video.frameOffset
        elem.getElementsByClassName("frames")[0].innerHTML = Math.ceil(video.duration * 30);
        elem.getElementsByClassName("duration")[0].innerHTML = video.duration;
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
        var start_val, diff=0;
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

let processor = {
    load: function() 
    {
        this.leftVideo              = document.getElementById("left");
        this.rightVideo             = document.getElementById("right");
        this.leftVideo.name         = "left";
        this.rightVideo.name        = "right";
        this.canvas                 = document.getElementById("adjusted-video");
        this.leftVideo.frameOffset  = 0;
        this.rightVideo.frameOffset = 0;
        this.leftVideo.maxFrame     = Math.ceil(this.leftVideo.duration * 30);
        this.rightVideo.maxFrame    = Math.ceil(this.rightVideo.duration * 30);
        this.leftVideo.adjustedFrameSize = this.leftVideo.maxFrame - this.leftVideo.frameOffset;
        this.rightVideo.adjustedFrameSize = this.rightVideo.maxFrame - this.rightVideo.frameOffset;
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
        
        function adjustTime(video) {return video.currentTime - (video.frameOffset / 30);}
        
        scrubber.value = Math.max(adjustTime(this.leftVideo)*30, adjustTime(this.rightVideo)*30);
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
        console.log(diff);
        this.leftVideo.currentTime = (diff + this.leftVideo.frameOffset)/30;
        console.log((diff + this.leftVideo.frameOffset)/30);
        this.rightVideo.currentTime = (diff + this.rightVideo.frameOffset)/30;
    }
}


