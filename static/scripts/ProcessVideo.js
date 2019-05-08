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
            self.rightVideo.pause();
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
    }
}