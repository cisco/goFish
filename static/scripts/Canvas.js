/**
 * @author Tomas Rigaux
 * @date May 29, 2019
 *
 * @file All of the various tools that can be used on the video canvas are
 * created here. Each tool is an object which has the ability to be rendered
 * onto the adjusted video canvas.
 */

'use strict';

///////////////////////////////////////////////////////////////////////////////
// Utility classes
///////////////////////////////////////////////////////////////////////////////

 /** Wrapper class for the various tool classes. */
class Toolkit
{
    /** Default constructor. Initializes all member variables. */
    constructor()
    {
        this.canvas         = $('#adjusted-video');
        this.mouse          = null;
        this.left_rulers    = null;
        this.right_rulers   = null;
        this.mode           = -1;
        this.toolButton     = null;
    }

    /** Sets which tool to use via a tool index.
     * @param {int} mode The index of the tool to bxe used.
     * @param {Object} button The button object which called this.
     */
    SetMode(mode, button)
    {
        if(mode == 1) 
        {
            this.ClearRulers();
            return;
        }

        if(this.mode != -1 && this.mode == mode)
            if(!button.classList.contains("active")) button.classList.add("active");
            else
            {
                button.classList.remove("active");
                this.mode = -1;
            }
        else
        {
            if(this.toolButton != null) this.toolButton.classList.remove("active");
            this.mode = this.mode != mode ? mode : -1;
            button.classList.add("active");
        }
        this.toolButton = button;
            
    }

    /** Utilize the functionalities of the tool at the selected index. */
    UseTool()
    {
        switch(this.mode)
        {
            case 0:
                this.AddRuler();
                break;
            case 1:
                this.ClearRulers();
                break;
            case -1:
            default:
                console.log("No tool currently active!");
        }

        // Refresh page elemnts to display changes.
        {
            var outputLen = document.getElementById("length"); 
            var length;
            var points = document.getElementsByClassName("points");
            var vectors = Array();
            if(points.length > 1)
            {
                vectors[0] = (points[0].innerHTML.replace("[", "").replace("]", "").split(" "));
                vectors[1] = (points[1].innerHTML.replace("[", "").replace("]", "").split(" "));

                length = Math.sqrt(Math.pow(vectors[1][0] - vectors[0][0], 2) + Math.pow(vectors[1][1] - vectors[0][1], 2) + Math.pow(vectors[1][2] - vectors[0][2], 2));
                outputLen.value = length;
            }
            // Reload the canvas to show rulers.
            this.canvas.load(window.location.href + " #adjusted-video > *");
        }
    }

    /** Adds a new ruler to the canvas. If the mouse is to the left, it creates
     * a left ruler. If it is on the right, a right ruler is created.
     */
    AddRuler()
    {
        var self = this;
        function PostResult(name, ruler, x_offset)
        {
            var line = ruler.line;
            if( line != null)
            {
                var sx = (line.start_point.x - x_offset);
                var ex = (line.end_point.x - x_offset);
                var data = " \""+name+"\" : { \"P0\" : {\"x\" :" + sx +", \"y\":"+(line.start_point.y - self.canvas.position().top)+"}, \"P1\" : {\"x\" :" + ex + ", \"y\":" + (line.end_point.y - self.canvas.position().top) + "}}";
                return data;
            }
            return null;
        }

        if(this.mouse.x <= this.canvas.position().left + this.canvas.width()/2)
        {
            if(this.left_rulers == null) 
                this.left_rulers = new Ruler(this.mouse.x, this.mouse.y, "#FF1144");
                if(this.left_rulers.line == null)
                    this.left_rulers.AddPoint(this.mouse.x, this.mouse.y);
        }
        else
        {
            if(this.right_rulers == null) 
                this.right_rulers = new Ruler(this.mouse.x, this.mouse.y, "#40e0d0");
            if(this.right_rulers.line == null)
                this.right_rulers.AddPoint(this.mouse.x, this.mouse.y);
        }

        function PostEvent(e) 
        {
            if (self.left_rulers != null && self.right_rulers != null)
            {
                var left = PostResult("keypoints_left", self.left_rulers, self.canvas.position().left);
                var right = PostResult("keypoints_right", self.right_rulers, (self.canvas.width() / 2) + self.canvas.position().left);
                var data = "{ " + left + ", " + right + " }";

                var xhr = new XMLHttpRequest();
                xhr.open("POST", "/processing/");
                xhr.setRequestHeader("Content-Type", "application/json");
                xhr.send(data);

                $("#info-panel").show().css({ top: self.mouse.y, left: self.mouse.x - 30 });
                setTimeout(function(){ 
                    $("#world-points").load(window.location.href + " #world-points > * ") 
                }, 1000);
            }
        }

        if (this.left_rulers != null && this.right_rulers != null)
        {
          $(document).on('points', PostEvent);
        }

        $(document).trigger("points");

    }

    /** Clear all rulers off of the screen. */
    ClearRulers()
    {
        if(this.left_rulers != null)
        {
            $("#ruler-"+this.left_rulers.ID).remove();
            this.left_rulers = null;
        }
        if(this.right_rulers != null)
        {
            $("#ruler-"+this.right_rulers.ID).remove();
            this.right_rulers = null;
        }
        $("#info-panel").hide();
    }

    /** Renders all in-use tools to the canvas. */
    Render()
    {
        var canvas = document.getElementById("adjusted-video");
        if(this.left_rulers != null) this.left_rulers.Render(canvas);
        if(this.right_rulers != null) this.right_rulers.Render(canvas);
    }
}

/** A helper class for storing a mouse object containing the current
 * position of the cursor on the screen.
 */
class Mouse
{
    /** Stores the mouse at a given origin.
     * @param {number} x The x coordinate of the cursor.
     * @param {number} y The y coordinate of the cursor.
     */
    constructor(x, y)
    {
        this.x = x;
        this.y = y;
    }
}


///////////////////////////////////////////////////////////////////////////////
// Tools
///////////////////////////////////////////////////////////////////////////////

/** Creates a ruler object, which allows for the dynamic creation of two
 * points and the line between them.
 */
class Ruler
{
    /** Constructor for the beginning point of the ruler. Also defines the 
     * colour of the rendered ruler.
     * @param {number} x The x coordinate of the origin of the ruler.
     * @param {number} y The y coordinate of the origin of the ruler.
     * @param {string} colour The colour to render the points and line of the ruler.
     */
    constructor(x, y, colour="#0011FF")
    {
        this.point_1 = null;
        this.point_2 = null;
        this.line    = null;
        this.colour  = colour;
        this.ID      = Math.round(Math.random() * 1000);
        $("#rulers").prepend('<div class="ruler" id="ruler-'+this.ID+'"></div>')
        $("#ruler-"+this.ID).css({background: this.colour});

    }

    /** Adds a new point for the ruler.
     * @param {number} x The x coordinate of the new point.
     * @param {number} y The y coordinate of the new point.
     */
    AddPoint(x, y)
    {
        if(this.point_1 != null && this.point_2 == null)
            this.point_2 = new Point(x, y, 2, this.ID);
        else if (this.point_2 == null)
            this.point_1 = new Point(x, y, 2, this.ID);
        if(this.point_1 != null & this.point_2 != null && this.line == null)
            this.line = new Line(this.point_1, this.point_2, this.ID)
    }

    /** Draws the endpoints and line to the canvas. */
    Render()
    {
        if(this.line != null) this.line.Render();
    }
}


///////////////////////////////////////////////////////////////////////////////
// Render Objects
///////////////////////////////////////////////////////////////////////////////

/** Base class for all objects which can be rendered onto a canvas. */
class RenderObject
{
    /** Default constructor. */
    constructor() {}

    /** Base super class render method.
     * @param {Object} canvas The canvas on which to render the object.
     */
    Render() {}
}

/** Creates a 2D point object, with x and y coordinates in a UV mapping
 * system.
 */
class Point extends RenderObject
{
    /** Constructor for the origin and radius of the point.
     * @param {number} x The x coordinate of the point.
     * @param {number} y The y coordinate of the point.
     */
    constructor(x, y, r, pID)
    {
        super();
        this.x = x;
        this.y = y;
        this.radius = r != null ? r : 2;
        this.pID = pID;
        this.ID =  Math.round(x) + "_" + Math.round(y);

        $("#ruler-" + this.pID).prepend('<div class="point" id="point-' + this.pID + '_' + this.ID + '"></div>');
        $("#point-" + this.pID + '_' + this.ID).css({
            top: this.y,
            left: this.x,
            width: this.radius * Math.PI,
            height: this.radius * Math.PI
        });

        var self = this;
        $('#point-' + this.pID + '_' + this.ID).draggable({
            drag: function(e, ui){
                self.x = $(this).position().left;
                self.y = $(this).position().top;
            }
        });
    }
}

/** A line between two given points in a UV mapping system. */
class Line extends RenderObject
{
    /** Default constructor for initializing all member variables.
     * @param {Point} p1 The start point for the line.
     * @param {Point} p2 The end poijt for the line.
     */
    constructor(p1, p2, pID)
    {
        super();
        this.start_point = p1;
        this.end_point   = p2;
        this.pID = pID;

        $("#ruler-" + this.pID).prepend('<div class="line" id="line-' + this.pID + '"></div>');

        var self = this;
        $("#point-"+this.pID+"_"+this.start_point.ID).draggable({
            drag: function(e, ui){
                self.start_point.x = $(this).position().left;
                self.start_point.y = $(this).position().top;
                self.Render();
            },
            stop: function(e, ui) {
                $(document).trigger("points");
            }
        });
        $("#point-"+this.pID+"_"+this.end_point.ID).draggable({
            drag: function(e, ui){
                self.end_point.x = $(this).position().left;
                self.end_point.y = $(this).position().top;
                self.Render();
            },
            stop: function(e, ui) {
                $(document).trigger("points");
            }
        });
    }

    /** Renders both endpoints and  the line between them. */
    Render()
    {
        if(this.start_point != null && this.end_point != null)
        {
            $('#line-' + this.pID).css({
                top: this.start_point.y,
                left: this.start_point.x,
                height: this.Length() + 'px',
                transform: "rotate(" + ((Math.atan2((this.start_point.y - this.end_point.y), (this.start_point.x - this.end_point.x)) + Math.PI/2) * 180 / Math.PI) + "deg)",
                transformOrigin: "0 0"
            });
        }
    }

    /** Gets the length of the vector that is the difference of the endpoints. */
    Length()
    {
        if(this.start_point != null && this.end_point != null)
        {
            this.vector = new Point(Math.abs(this.end_point.x - this.start_point.x), Math.abs(this.end_point.y - this.start_point.y));
            return Math.round(Math.sqrt(Math.pow(this.vector.x, 2) + Math.pow(this.vector.y, 2)) * 1000) / 1000;
        }
    }
}
