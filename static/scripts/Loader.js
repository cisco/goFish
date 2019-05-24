let processor = new Processor();
let videoHandler = new VideoHandler(processor);
let eventHandler = new EventHandler();

var timer = null;
let FRAMERATE = 30;

$(function(){
    SubmitForms();
});

$(document).ajaxSuccess(function(){
    SubmitForms();
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

function LoadAll()
{
    processor = new Processor();
    videoHandler = new VideoHandler(processor);
    eventHandler = new EventHandler();


    // Handle JSON for Left Video
    if(document.getElementById("left-json") != null)
    {
        var json = new JsonHandler(document.getElementById("left-json").textContent, processor.leftVideo);
        if(json.handle != null)
            if(json.Event_QRCode() != null)
            videoHandler.seekFrame(processor.leftVideo, json.Event_QRCode().frame);

        eventHandler.events = json.events;
    }

    // Handle JSON for Right Video.
    if(document.getElementById("right-json") != null)
    {
        var json = new JsonHandler(document.getElementById("right-json").textContent, processor.rightVideo);
        if(json.handle != null)
            if(json.Event_QRCode() != null)
            videoHandler.seekFrame(processor.rightVideo, json.Event_QRCode().frame);
        eventHandler.events = json.events;
    }

    eventHandler.draw();
}

function SubmitForms()
{
    $("#select-video, #upload-video").on("submit", function(e){
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