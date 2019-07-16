////////////////////////////////////////////////////////////////////////////////////////////
// Setup global variables
////////////////////////////////////////////////////////////////////////////////////////////
let videoHandler = new VideoHandler();
let eventHandler = new EventHandler();
let toolkit      = new Toolkit();

var timer   = null;
var playing = false;

////////////////////////////////////////////////////////////////////////////////////////////
// Loading Methods
////////////////////////////////////////////////////////////////////////////////////////////

/// Document Ready
/// \brief Run when the document is first loaded.
$(function(){
    // Init jQuery events.
    {
        SubmitMultipartForms("#upload-video, #calibrate-cameras");

        $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
        $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));

        $(".file-item").on("click", function(e){
            $.post("/video/", JSON.stringify({file : $(this).html()}), function(response){ $(".container").html($('<div />').append(response).find(".container").html()); Refresh(e);});
        });
    }

    // Startup timers.
    {
        setInterval(function(){
            $("#world-points").load(window.location.href + " #world-points > * "); 
        }, 1000);

        setInterval(function(){
            $("#files").load(window.location.href + " #files > *");
            $("#processes").load(window.location.href + " #processes > *");
        }, 5000);
    }
});

/// AJAX Complete
/// \brief Runs after every completed AJAX request.
$(document).ajaxComplete(function(){
    $(".file-item").on("click", function(e){
        e.preventDefault();
        e.stopImmediatePropagation();
        $.post("/video/", JSON.stringify({file : $(this).html()}), function(response){ $(".container").html($('<div />').append(response).find(".container").html()); Refresh(e);});
    });
});


////////////////////////////////////////////////////////////////////////////////////////////
// Helper Loading methods
////////////////////////////////////////////////////////////////////////////////////////////

/// LoadAll
/// \brief Loads all necessary video processing objects with data obtained from the server.
function LoadAll()
{
    // Reinitialize variables.
    videoHandler    = new VideoHandler();
    eventHandler    = new EventHandler();
    toolkit         = new Toolkit();

    // Handle JSON for Video.
    if(document.getElementById("json-config") != null)
    {
        var json = new JsonHandler(document.getElementById("json-config").textContent, videoHandler.video);
        eventHandler.events = json.events;
    }

    // Draw events on the scrubber bar.
    eventHandler.draw();
}

/// Refresh
/// \brief Refreshes specific elements on the oage to update them with new content.
function Refresh(e){
    e.preventDefault();

    // Adjust main canvas for correct aspect ratio.
    {
        $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
        $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));
    }

    // Click Events
    {
        $("#event-time-bar").on("click", function(e){
            var bar_offset = e.clientX - $(this).position().left;
            var scrubber = document.getElementById("event-time-bar");
            var rel_pos = Math.round( (bar_offset / $(this).width()) * scrubber.max );
            scrubber.value = rel_pos;
            videoHandler.video.currentTime = scrubber.value / FRAMERATE;
            setTimeout(Redraw, 300);
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
    }

    // Init jQuery Windows.
    {
        $("#tools").draggable();
        $("#info-panel").draggable();
    }


    setTimeout(LoadAll, 1000);
}

/// SubmitMultipartForms
/// \brief Submits multipart file forms using AJAX.
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
                Refresh(e)
            }
        });
    });
}

/// HandleTools
/// \brief Sets up functionality for the toolbar.
function HandleTools()
{
    $(document).mousemove(function(e){
        toolkit.mouse = new Mouse(e.pageX, e.pageY);
    });

    toolkit.canvas.on("click", function(e) {
        toolkit.UseTool();
        Redraw();
    });

    $("#tools").draggable();
    $("#info-panel").draggable();
}

/// Redraw
/// \brief Redraws the main video and scrubber bar canvases.
function Redraw()
{
    videoHandler.draw();
    eventHandler.draw();
    toolkit.Render();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Video Control methods
////////////////////////////////////////////////////////////////////////////////////////////
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
            toolkit.Render();
            $("#total-events").html(eventHandler.events.length);

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
