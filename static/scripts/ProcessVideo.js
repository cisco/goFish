let processor = {
    doLoad: function() 
    {
        this.leftVideo = document.getElementById("left");
        this.rightVideo = document.getElementById("right");
        this.canvas = document.getElementById("left-right-video");
        let self = this;
        this.leftVideo.addEventListener('play', function(){
            //if(self.rightVideo.paused) self.rightVideo.play();
        }, false);
        this.rightVideo.addEventListener('play', function(){
            //if(self.leftVideo.paused) self.leftVideo.play();
            self.timerCallback();
        }, false);

        this.leftVideo.addEventListener('pause', function(){
            //if(!self.ended) self.rightVideo.pause();
        }, false);
        this.rightVideo.addEventListener('pause', function(){
            //if(!self.ended) self.leftVideo.pause();
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

        var time = Math.min(this.leftVideo.duration - lOffset, this.rightVideo.duration - rOffset);

        var scrubber = document.getElementById("scrubber");
        scrubber.max = time;
        scrubber.value = Math.max(this.leftVideo.currentTime - lOffset, this.rightVideo.currentTime - rOffset);
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
    },

    sync: function()
    {
        this.pause();
        this.leftVideo.currentTime = parseFloat(document.getElementById("left-offset").innerText);
        this.rightVideo.currentTime = parseFloat(document.getElementById("right-offset").innerText);
        this.play();
    },

    adjustVideo: function(diff)
    {
        var lOffset = parseFloat(document.getElementById("left-offset").innerText);
        var rOffset = parseFloat(document.getElementById("right-offset").innerText);

        this.leftVideo.currentTime = (diff * this.leftVideo.duration) + lOffset;
        this.rightVideo.currentTime = (diff * this.rightVideo.duration) + rOffset;
    }
}