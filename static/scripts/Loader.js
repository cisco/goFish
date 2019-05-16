$(function(){
    SubmitForms();
});

$(document).ajaxSuccess(function(){
    SubmitForms();
    setTimeout(load, 1000);
});

function load()
{   
    // Start up the processor and video_handler.
    processor.load();
    video_handler.handleSlider(processor.leftVideo);
    video_handler.handleSlider(processor.rightVideo);
    video_handler.adjustMainVideo();
    video_handler.timerCallback();

    // Handle JSON for Left Video
    if(document.getElementById("left-json") != null)
        json_handler.load(document.getElementById("left-json").textContent);
    if(json_handler.handle != null)
    {
        if(json_handler.Event_QRCode() != null)
            video_handler.seekFrame(processor.leftVideo, json_handler.Event_QRCode().frame);
        json_handler.timerCallback(processor.leftVideo);
    }
    // handle JSON for Right Video.
    if(document.getElementById("right-json") != null)
        json_handler.load(document.getElementById("right-json").textContent);
    if(json_handler.handle != null)
    {
        if(json_handler.Event_QRCode() != null)
            video_handler.seekFrame(processor.rightVideo, json_handler.Event_QRCode().frame);
        json_handler.timerCallback(processor.rightVideo);
    }
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