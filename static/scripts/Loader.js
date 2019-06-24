let videoHandler = new VideoHandler();
let eventHandler = new EventHandler();
let toolkit      = new Toolkit();

var timer   = null;
var playing = false;

$(function(){
    SubmitMultipartForms("#select-video, #upload-video");
    HandleTools();
    $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
    $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));

    $(".file-item").on("click", function(e){
        $.post("/", JSON.stringify({file : $(this).html()}), function(response){ $(".container").html($('<div />').append(response).find(".container").html());});
    });
});

$(document).ajaxComplete(function(e){
    e.preventDefault();
    e.stopImmediatePropagation();

    alert("");

    $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
    $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));

    $("#tools").draggable();
    $("#info-panel").draggable();
    //$("#info-panel").resizable();

    $("#event-time-bar").on("click", function(e){
        var bar_offset = e.clientX - $(this).position().left;
        var scrubber = document.getElementById("event-time-bar");
        var rel_pos = Math.round( (bar_offset / $(this).width()) * scrubber.max );
        scrubber.value = rel_pos;
        videoHandler.video.currentTime = scrubber.value / FRAMERATE;
        setTimeout(ReDraw, 300);
    });

    $("#play-pause").on("click", function(e){
        if(!$(this).hasClass("active"))
        {
            $(this).addClass("active");
            $(this).children(".material-icons").html("&#xe034;");
            Play();
        }
        else
        {
            $(this).removeClass("active");
            $(this).children(".material-icons").html("&#xe037;");
            Pause();
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
    SubmitMultipartForms("#select-video, #upload-video");
    HandleTools();
    $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
    $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));
    LoadAll();
}

function Play()
{
    if(videoHandler.video != null)
    {
        if(videoHandler.ended)
            eventHandler.event_index = 0;

        videoHandler.play();

        timer = setInterval(function(){
            videoHandler.run();

            eventHandler.run();
            for(var i = 0; i < toolkit.left_rulers.length; i++)
                toolkit.left_rulers[i].Render(videoHandler.canvas);
            for(var i = 0; i < toolkit.right_rulers.length; i++)
                toolkit.right_rulers[i].Render(videoHandler.canvas);

            $("#total-events").html(eventHandler.events.length);
            GetRulers();

        }, 1000/FRAMERATE);
    }
}

function Pause()
{
    if(videoHandler.video != null)
    {
        clearInterval(timer);
        videoHandler.pause();
    }
}

function SeekBack()
{
    if(videoHandler.video != null)
    {
        videoHandler.adjustVideo(-30);
        setTimeout(function(){
            videoHandler.draw();
            eventHandler.draw();
        }, 300);
    }
}

function ReDraw()
{
    videoHandler.draw();
    eventHandler.draw();
    for(var i = 0; i < toolkit.left_rulers.length; i++)
        toolkit.left_rulers[i].Render(videoHandler.canvas);
    for(var i = 0; i < toolkit.right_rulers.length; i++)
        toolkit.right_rulers[i].Render(videoHandler.canvas);
}

function SeekForward()
{
    if(videoHandler.video != null)
    {
        videoHandler.adjustVideo(30);
        setTimeout(function(){
            videoHandler.draw();
            eventHandler.draw();
        }, 300);
    }
}

function LoadAll()
{
    videoHandler    = new VideoHandler();
    eventHandler    = new EventHandler();
    toolkit         = new Toolkit();

    // Handle JSON for Video.
    if(document.getElementById("json-config") != null)
    {
        var json = new JsonHandler(document.getElementById("json-config").textContent, videoHandler.video);
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
    //$("#info-panel").resizable();
}

function GetRulers()
{
    for(var i = 0; i < toolkit.left_rulers.length && toolkit.left_rulers.length == toolkit.right_rulers.length; i++)
    {
        var str="x1=" + (toolkit.left_rulers[i].point_1.x) + ", y1=" + (toolkit.left_rulers[i].point_1.y);
        console.log(str);
    }
}