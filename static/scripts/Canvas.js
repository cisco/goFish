/**
 * @author Tomas Rigaux
 * @date May 29, 2019
 *
 * @file All of the various tools that can be used on the video canvas are
 * created here. Each tool is an object which has the ability to be rendered
 * onto the adjusted video canvas.
 */

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
     * @param {} mode The index of the tool to bxe used.
     * @param {} button The button object which called this.
     */
    SetMode(mode, button)
    {
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
        function PostResult(name, ruler, x_offset)
        {
            var line = ruler.line;
            var sx = (line.start_point.x - x_offset);
            var ex = (line.end_point.x - x_offset);
            var data = " \""+name+"\" : { \"P0\" : {\"x\" :" + sx +", \"y\":"+line.start_point.y+"}, \"P1\" : {\"x\" :" + ex + ", \"y\":" + line.end_point.y + "}}";
            return data;
        }

        if(this.mouse.x <= this.canvas.position().left + this.canvas.width()/2)
        {
            if(this.left_rulers == null) 
                this.left_rulers = new Ruler(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top, "#FF1144");
            this.left_rulers.AddPoint(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top);
        }
        else
        {
            if(this.right_rulers == null) 
                this.right_rulers = new Ruler(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top, "#40e0d0");
            this.right_rulers.AddPoint(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top);
        }

        if(this.left_rulers != null && this.right_rulers != null)
            if(this.left_rulers.point_2 != null && this.right_rulers.point_2 != null)
            {
                var left = PostResult("keypoints_left", this.left_rulers, 0);
                var right = PostResult("keypoints_right", this.right_rulers, this.canvas.width()/2);
                var data = "{ " + left + ", " + right + " }";

                var xhr = new XMLHttpRequest();
                xhr.open("POST", "/processing/");
                xhr.setRequestHeader("Content-Type", "application/json");                
                xhr.send(data);

                $("#info-panel").show().css({top:this.mouse.y, left: this.mouse.x-30});
            }
    }

    /** Clear all rulers off of the screen. */
    ClearRulers()
    {
        if(this.left_rulers != null) this.left_rulers = null;
        if(this.right_rulers != null) this.right_rulers = null;
    }

    /** Renders all in-use tools to the canvas. */
    Render()
    {
        var canvas = document.getElementById("adjusted-video");
        if(this.left_rulers != null) this.left_rulers.Render(canvas);
        if(this.right_rulers != null) this.right_rulers.Render(canvas);
    }
}

/** \brief A helper class for storing a mouse object containing the current
 *        position of the cursor on the screen.
 */
class Mouse
{
    /** Stores the mouse at a given origin.
     * @param {} x The x coordinate of the cursor.
     * @param {} y The y coordinate of the cursor.
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
     * @param {} x The x coordinate of the origin of the ruler.
     * @param {} y The y coordinate of the origin of the ruler.
     * @param {} colour The colour to render the points and line of the ruler.
     */
    constructor(x, y, colour="#0011FF")
    {
        this.point_1 = null;
        this.point_2 = null;
        this.line    = new Line(this.point_1, this.point_2);
        this.colour  = colour;
    }

    /** Adds a new point for the ruler.
     * @param {} x The x coordinate of the new point.
     * @param {} y The y coordinate of the new point.
     */
    AddPoint(x, y)
    {
        if(this.point_1 != null)
        {
            this.point_2 = new Point(x, y);
            this.line.end_point = this.point_2;
        }
        else
        {
            this.point_1 = new Point(x, y);
            this.line.start_point = this.point_1;
        }
    }

    /** Draws the endpoints and line to the canvas.
     * @param {} canvas The canvas on which to draw the ruler.
     */
    Render(canvas)
    {
        var context = canvas.getContext('2d');
        context.fillStyle = this.colour;
        context.strokeStyle = this.colour;
        
        if(this.line != null) this.line.Render(canvas);
        if(this.point_1 != null) this.point_1.Render(canvas);
        if(this.point_2 != null) this.point_2.Render(canvas);
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
     * @param {} canvas The canvas on which to render the object.
     */
    Render(canvas) {}
}

/** Creates a 2D point object, with x and y coordinates in a UV mapping
 * system.
 */
class Point extends RenderObject
{
    /** Constructor for the origin and radius of the point.
     * @param {} x The x coordinate of the point.
     * @param {} y The y coordinate of the point.
     */
    constructor(x, y, r)
    {
        super();
        this.x      = x;
        this.y      = y;
        this.radius = r != null ? r : 2;
    }

    /** Draws the point to the canvas.
     * @param {} canvas The canvas on which to render the point.
     */
    Render(canvas)
    {
        var context = canvas.getContext("2d");

        context.beginPath();
        context.arc(this.x, this.y, this.radius, 0, 2 * Math.PI, false);
        context.fill();
        context.stroke();
    }
}

/** A line between two given points in a UV mapping system. */
class Line extends RenderObject
{
    /** Default constructor for initializing all member variables.
     * @param {} p1 The start point for the line.
     * @param {} p2 The end poijt for the line.
     */
    constructor(p1, p2)
    {
        super();
        this.start_point = p1;
        this.end_point   = p2;
    }

    /** Renders both endpoints and  the line between them.
     * @param {} canvas The canvas on which to render the line.
     */
    Render(canvas)
    {
        if(this.start_point != null && this.end_point != null)
        {
            var ctx = canvas.getContext('2d');

            ctx.beginPath(); 
            ctx.moveTo(this.start_point.x, this.start_point.y);
            ctx.lineTo(this.end_point.x, this.end_point.y);

            this.Length();
            var u = new Point(this.vector.x / this.Length(), this.vector.y / this.Length());
            var d= this.Length()/2;
            ctx.fillText(this.Length(), Math.min(this.start_point.x, this.end_point.x) + (d*u.x) + 5,
                                        Math.min(this.start_point.y, this.end_point.y) + (d*u.y) - 5);
            ctx.stroke();
            console.log("I'm still printing!");
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
