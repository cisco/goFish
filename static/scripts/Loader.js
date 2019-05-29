let processor    = new Processor();
let videoHandler = new VideoSettingsHandler(processor);
let eventHandler = new EventHandler();

var timer = null;

$(function(){
    SubmitMultipartForms("#select-video, #upload-video");
});

$(document).ajaxSuccess(function(){
    SubmitMultipartForms("#select-video, #upload-video");
    $("#event-time-bar").on("click", function(e){
        var bar_offset = e.clientX - $(this).position().left;
        var scrubber = document.getElementById("event-time-bar");
        var rel_pos = Math.round( (bar_offset / $(this).width()) * scrubber.max );
        console.log(rel_pos);
        scrubber.value = rel_pos;
        processor.leftVideo.currentTime = (scrubber.value + processor.leftVideo.frameOffset) / 30;
        processor.rightVideo.currentTime = (scrubber.value + processor.rightVideo.frameOffset) / 30;
        setTimeout(function(){
            processor.draw();
            eventHandler.draw();
        }, 300);
    });
    setTimeout(LoadAll, 1000);
});

window.onload = function()
{
    LoadAll();
}

function play()
{
    processor.play();

    timer = setInterval(function(){
        processor.run();
        videoHandler.run();
        eventHandler.run();
    }, 1000/FRAMERATE);
}

function pause()
{
    processor.pause();
    clearInterval(timer);
}

function sync()
{
    processor.sync();
    
    timer = setInterval(function(){
        processor.run();
        videoHandler.run();
        eventHandler.run();
    }, 1000/FRAMERATE);

}

function seekBack()
{
    videoHandler.seekFrame(processor.leftVideo, -1);
    videoHandler.seekFrame(processor.rightVideo, -1);
    setTimeout(function(){
        processor.draw();
        eventHandler.draw();
    }, 300);
}

function seekForward()
{
    videoHandler.seekFrame(processor.leftVideo, 1);
    videoHandler.seekFrame(processor.rightVideo, 1);
    setTimeout(function(){
        processor.draw();
        eventHandler.draw();
    }, 300);
}

function LoadAll()
{
    processor = new Processor();
    videoHandler = new VideoSettingsHandler(processor);
    eventHandler = new EventHandler();


    // Handle JSON for Left Video
    if(document.getElementById("left-json") != null)
    {
        var json = new JsonHandler(document.getElementById("left-json").textContent, processor.leftVideo);
        if(json.handle != null)
            if(json.Event_QRCode() != null)
            videoHandler.seekFrameOffset(processor.leftVideo, json.Event_QRCode().frame);

        eventHandler.events = json.events;
    }

    // Handle JSON for Right Video.
    if(document.getElementById("right-json") != null)
    {
        var json = new JsonHandler(document.getElementById("right-json").textContent, processor.rightVideo);
        if(json.handle != null)
            if(json.Event_QRCode() != null)
            videoHandler.seekFrameOffset(processor.rightVideo, json.Event_QRCode().frame);
        eventHandler.events = json.events;
    }

    eventHandler.draw();
}

function SubmitMultipartForms(forms)
{
    $(forms).on("submit", function(e){
        e.preventDefault();

        var data = new FormData($(this)[0]);

        $.ajax({
            type: $(this).attr('method'),
            url: $(this).attr('action'),
            enctype: 'multipart/form-data',
            data: data,
            processData: false,
            contentType: false,
            cache: false,
            success: function(response){
                $(".container").html($('<div />').append(response).find(".container").html());
            }
        });
    });
}