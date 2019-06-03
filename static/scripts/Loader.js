let processor    = new Processor();
let videoHandler = new VideoSettingsHandler(processor);
let eventHandler = new EventHandler();
let toolkit      = new Toolkit();

var timer   = null;
var playing = false;

$(function(){
    SubmitMultipartForms("#select-video, #upload-video");
    HandleTools();
    $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
    $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));
});

$(document).ajaxComplete(function(e){
    e.preventDefault();
    e.stopImmediatePropagation();

    $("#tools").draggable();
    $("#info-panel").draggable();
    $("#info-panel").resizable();

    $("#event-time-bar").on("click", function(e){
        var bar_offset = e.clientX - $(this).position().left;
        var scrubber = document.getElementById("event-time-bar");
        var rel_pos = Math.round( (bar_offset / $(this).width()) * scrubber.max );
        scrubber.value = rel_pos;
        processor.leftVideo.currentTime = (scrubber.value + processor.leftVideo.frameOffset) / 30;
        processor.rightVideo.currentTime = (scrubber.value + processor.rightVideo.frameOffset) / 30;
        setTimeout(ReDraw, 300);
    });

    $("#play-pause").on("click", function(e){
        if(!$(this).hasClass("active"))
        {
            $(this).addClass("active");
            $(this).children(".material-icons").html("&#xe034;");
            play();
        }
        else
        {
            $(this).removeClass("active");
            $(this).children(".material-icons").html("&#xe037;");
            pause();
        }
    });

    $("#toolbar-shape").on("click", function(e){
        if(!$("#tools").hasClass("vertical"))
        {
            $("#tools").addClass("vertical");
            $(this).children(".material-icons").html("&#xe5cb;");
        }
        else
        {
            $("#tools").removeClass("vertical");
            $(this).children(".material-icons").html("&#xe5cf;");
        }
    });

    setTimeout(LoadAll, 1000);
});

window.onload = function()
{
    LoadAll();
}

function play()
{
    if(processor.ended)
        eventHandler.event_index = 0;
    processor.play();

    timer = setInterval(function(){
        processor.run();
        videoHandler.run();
        eventHandler.run();
        for(var i = 0; i < toolkit.left_rulers.length; i++)
            toolkit.left_rulers[i].Render(processor.canvas);
        for(var i = 0; i < toolkit.right_rulers.length; i++)
            toolkit.right_rulers[i].Render(processor.canvas);

        $("#total-events").html(eventHandler.events.length);
        GetRulers();

    }, 1000/FRAMERATE);
}

function pause()
{
    clearInterval(timer);
    processor.pause();
}

function sync()
{
    eventHandler.event_index = 0;
    processor.sync();
    
    if(playing)
        timer = setInterval(function(){
            processor.run();
            videoHandler.run();
            eventHandler.run();
            for(var i = 0; i < toolkit.left_rulers.length; i++)
                toolkit.left_rulers[i].Render(processor.canvas);
            for(var i = 0; i < toolkit.right_rulers.length; i++)
                toolkit.right_rulers[i].Render(processor.canvas);
    
            $("#total-events").html(eventHandler.events.length);
            GetRulers();
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
    processor       = new Processor();
    videoHandler    = new VideoSettingsHandler(processor);
    eventHandler    = new EventHandler();
    toolkit         = new Toolkit();

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
        eventHandler.events.concat(json.events);
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

function HandleTools()
{
    $(document).mousemove(function(e){
        toolkit.mouse = new Mouse(e.pageX, e.pageY);
    });

    $(document).on('mouseup', function(e){
        toolkit.UseTool();
        ReDraw();
    });

    $("#tools").draggable();
    $("#info-panel").draggable();
    $("#info-panel").resizable();
}

function ReDraw()
{
    processor.draw();
    eventHandler.draw();
    for(var i = 0; i < toolkit.left_rulers.length; i++)
        toolkit.left_rulers[i].Render(processor.canvas);
    for(var i = 0; i < toolkit.right_rulers.length; i++)
        toolkit.right_rulers[i].Render(processor.canvas);
}

function GetRulers()
{
    for(var i = 0; i < toolkit.left_rulers.length && toolkit.left_rulers.length == toolkit.right_rulers.length; i++)
    {
        var str="x1=" + (toolkit.left_rulers[i].point_1.x) + ", y1=" + (toolkit.left_rulers[i].point_1.y);
        console.log(str);
    }
}