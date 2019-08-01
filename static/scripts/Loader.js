/**
 * @author Tomas Rigaux
 * @date May 16, 2019
 *
 * @file Loader.js handles most of the jQuery related commands that help keep
 * the page Asynchronous with loading in new information from the server-side
 * Go template code.
 */

///////////////////////////////////////////////////////////////////////////////
// Setup global variables
///////////////////////////////////////////////////////////////////////////////
let videoHandler = new VideoHandler();
let eventHandler = new EventHandler();
let toolkit      = new Toolkit();

var timer   = null;
var playing = false;

///////////////////////////////////////////////////////////////////////////////
// Loading Methods
///////////////////////////////////////////////////////////////////////////////

/** Run when the document is first loaded. */
$(function(){
    // Init jQuery events.
    {
        SubmitMultipartForms("#upload-video, #calibrate-cameras");

        $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
        $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));

        $(".file-item").on("click", function(e){
            $.post("/video/", JSON.stringify({file : $(this).html()}), function(response){ $(".container").html($('<div />').append(response).find(".container").html()); Refresh(e);});
        });

        $("#show-process").on("click", function(e){
            $("#processes").slideToggle();
        });
        
        $(".loader").hide();
    }

    // Startup timers.
    {
        setInterval(function(){
            $("#files").load(window.location.href + " #files > *");
            $("#processes").load(window.location.href + " #processes > *");
        }, 5000);
    }

    // Navbar gradient effect.
    {
        lightColor = "#57585b";
        gradientSize = 200;
        var originalBG = $(".navbar").css("background-color");
        $('.navbar').mousemove(function(e) {
            x  = e.pageX - this.offsetLeft;
            y  = e.pageY - this.offsetTop;
            xy = x + " " + y;
            
            var bgWebKit = "-webkit-gradient(radial, " + xy + ", 0, " + xy + ", " + gradientSize + ", from(" + lightColor + "), to(rgba(255,255,255,0.0))), " + originalBG;
            var bgMoz    = "-moz-radial-gradient(" + x + "px " + y + "px 45deg, circle, " + lightColor + " 0%, " + originalBG + " " + gradientSize + "px)";
                                
            $(this)
                .css({ background: bgWebKit })
                .css({ background: bgMoz });         
        }).mouseleave(function() {
                $(this).css({ background: originalBG });
        });
    }

    {
        var inputs = document.querySelectorAll( 'input[type=file]' );
        Array.prototype.forEach.call( inputs, function( input )
        {
            var label	 = input.nextElementSibling,
                labelVal = label.innerHTML;

            function GetFiles(e)
            {
                console.log("Hi");
                var fileName = '';
                if( this.files && this.files.length > 1 )
                    fileName = ( this.getAttribute( 'data-multiple-caption' ) || '' ).replace( '{count}', this.files.length );

                if( fileName )
                    label.innerHTML = fileName;
                else
                    label.innerHTML = labelVal;
            }
            input.addEventListener('load', GetFiles);
            input.addEventListener('change', GetFiles);
        });
    }

    setTimeout(LoadAll, 1000);
});

$(window).resize(function(){
    $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
    $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width") / 8 * 3));
})

/** Runs after every completed AJAX request. */
$(document).ajaxComplete(function(){
    $(".file-item").on("click", function(e){
        e.preventDefault();
        e.stopImmediatePropagation();
        $.post("/video/", JSON.stringify({file : $(this).html()}), function(response){ $(".container").html($('<div />').append(response).find(".container").html()); Refresh(e);});
    });

});


///////////////////////////////////////////////////////////////////////////////
// Helper Loading methods
///////////////////////////////////////////////////////////////////////////////

/** Loads all necessary video processing objects with data obtained from the 
 * server.
 */
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
    HandleTools();
    
}

/** Refreshes specific elements on the oage to update them with new content.
 * @param {Event} e The page event object.
 */
function Refresh(e){
    e.preventDefault();

    // Adjust main canvas for correct aspect ratio.
    {
        $("#adjusted-video").attr("width", $("#adjusted-video").parent().width());
        $("#adjusted-video").attr("height", ($("#adjusted-video").attr("width")/8*3));
    }

    // Click Events
    {
        $("#event-time-bar").on("click", function(e){
            var barOffset = e.clientX - $(this).position().left;
            var scrubber = document.getElementById("event-time-bar");
            var relPos = Math.round((barOffset/$(this).width())*scrubber.max);
            scrubber.value = relPos;
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

    setTimeout(LoadAll, 1000);
}

/** Submits multipart file forms using AJAX.
 * @param {string} forms The IDs of all the forms that use this to be submitted (comma separated).
 */
function SubmitMultipartForms(forms)
{
    $(forms).on("submit", function(e){
        e.preventDefault();

        $(this).hide();
        $(".loader").show();

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
                $(forms).show();
                $(".loader").hide();
                $(".container").html($('<div />').append(response).find(".container").html());
                Refresh(e)
            }
        });
    });
}

/** Sets up functionality for the toolbar. */
function HandleTools()
{
    $(document).mousemove(function(e){
        toolkit.mouse = new Mouse(e.pageX, e.pageY);
    });

    toolkit.canvas.on("click", function(e) {
        toolkit.UseTool();
        Redraw();
    });
}

/** Redraws the main video and scrubber bar canvases. */
function Redraw()
{
    videoHandler.draw();
    eventHandler.draw();
    toolkit.Render();
}


///////////////////////////////////////////////////////////////////////////////
// Video Control methods
///////////////////////////////////////////////////////////////////////////////

/** Starts playing all video handling elements. */
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

/** Pauses all video handling elements. */
function Pause()
{
    if(videoHandler.video != null)
    {
        clearInterval(timer);
        videoHandler.pause();
    }
}

/** Seeks back 1 second of the video. */
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

/** Seeks forward 1 second of the video. */
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
