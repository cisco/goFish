let processor = {
    doLoad: function() 
    {
        this.leftVideo = document.getElementById("left");
        this.rightVideo = document.getElementById("right");
        this.canvas = document.getElementById("left-right-video");
        let self = this;
        this.leftVideo.addEventListener('play', function(){
            self.rightVideo.play();
        }, false);
        this.rightVideo.addEventListener('play', function(){
            //self.leftVideo.currenttime = this.currenttime;
            self.timerCallback();
        }, false);

        this.leftVideo.addEventListener('pause', function(){
            if(!self.ended) self.rightVideo.pause();
        }, false);
        this.rightVideo.addEventListener('pause', function(){
            self.timerCallback();
            clearTimeout(this.timer);
        }, false);
    },

    timerCallback: function()
    {
        if((this.leftVideo.paused && this.rightVideo.paused) ||
        (this.leftVideo.ended && this.rightVideo.ended))
        return;

        this.draw();
        let self = this;
        this.timer = setTimeout(function(){self.timerCallback()}, 0);
    },

    draw: function()
    {
        var context = this.canvas.getContext('2d');

        // TODO: Replace these with a function that will draw the exact pixels with minimum loss.
        context.drawImage(this.leftVideo, 0, 0, this.canvas.width/2, this.canvas.height);
        context.drawImage(this.rightVideo, this.canvas.width/2, 0, this.canvas.width/2, this.canvas.height);

        // Calculate the length of the combined video.
        var lOffset = parseFloat(document.getElementById("left-offset").innerText);
        var rOffset = parseFloat(document.getElementById("right-offset").innerText);
        var time = ((this.leftVideo.duration - lOffset) + (this.rightVideo.duration - rOffset)) /2

        var scrubber = document.getElementById("curr-time");
        scrubber.max = time.toString();
        var max_curr = Math.max(this.leftVideo.currentTime, this.rightVideo.currentTime);
        var min_curr = Math.min(this.leftVideo.currentTime, this.rightVideo.currentTime)
        var max_offset = Math.max(lOffset, rOffset), min_offset = Math.min(lOffset, rOffset);
        scrubber.value = ((max_curr - max_offset) + (min_curr - min_offset)) / 2;
    },

    pause : function()
    {
        this.leftVideo.pause();
        this.rightVideo.pause();
    },

    play: function()
    {
        this.leftVideo.play();
        this.rightVideo.play();
    }
}