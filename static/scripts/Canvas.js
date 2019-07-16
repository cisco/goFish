class Toolkit
{
    constructor()
    {
        this.canvas         = $('#adjusted-video');
        this.mouse          = null;
        this.left_rulers    = null;
        this.right_rulers   = null;
        this.mode           = -1;
        this.toolButton     = null;
    }

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
                outputLen.innerHTML = length;
            }
        }
    }

    AddRuler()
    {
        // TODO: Refactor this and in GoFish.go to respectively submit and receive both rulers simultaneously.
        function PostResult(ruler, url, x_offset)
        {
            var line = ruler.line;
            var sx = (line.start_point.x - x_offset);
            var ex = (line.end_point.x - x_offset);
            var data = "{ \"P0\" : {\"x\" :" + sx +", \"y\":"+line.start_point.y+"}, \"P1\" : {\"x\" :" + ex + ", \"y\":" + line.end_point.y + "}}";
            
            var xhr = new XMLHttpRequest();
            xhr.open("POST", url);
            xhr.setRequestHeader("Content-Type", "application/json");                
            xhr.send(data);
        }

        if(this.mouse.x <= this.canvas.position().left + this.canvas.width()/2)
        {
            if(this.left_rulers == null) 
                this.left_rulers = new Ruler(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top, "#FF1144");
            this.left_rulers.AddPoint(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top);
            if(this.left_rulers.point_2 != null)
            {
                PostResult(this.left_rulers, "/processing/left", 0);
            }
        }
        else
        {
            if(this.right_rulers == null) 
                this.right_rulers = new Ruler(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top, "#40e0d0");
            this.right_rulers.AddPoint(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top);
            if(this.right_rulers.point_2 != null)
            {
                PostResult(this.right_rulers, "/processing/right", this.canvas.width() / 2);
            }
        }
    }

    ClearRulers()
    {
        if(this.left_rulers != null) this.left_rulers = null;
        if(this.right_rulers != null) this.right_rulers = null;
    }

    Render()
    {
        var canvas = document.getElementById("adjusted-video");
        if(this.left_rulers != null) this.left_rulers.Render(canvas);
        if(this.right_rulers != null) this.right_rulers.Render(canvas);
        this.canvas.load(window.location.href + " #adjusted-video > *");
    }
}

class RenderObject
{
    constructor() {}

    Render(canvas) {}
}

class Point extends RenderObject
{
    constructor(x, y, r)
    {
        super();
        this.x      = x;
        this.y      = y;
        this.radius = r != null ? r : 2;
        //console.log("x=", this.x, ", y=", this.y);
    }

    Render(canvas)
    {
        var context = canvas.getContext("2d");

        context.beginPath();
        context.arc(this.x, this.y, this.radius, 0, 2 * Math.PI, false);
        context.fill();
        context.stroke();
    }
}

class Line extends RenderObject
{
    constructor(p1, p2)
    {
        super();
        this.start_point = p1;
        this.end_point   = p2;
    }

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
        }
    }

    Length()
    {
        if(this.start_point != null && this.end_point != null)
        {
            this.vector = new Point(Math.abs(this.end_point.x - this.start_point.x), Math.abs(this.end_point.y - this.start_point.y));
            return Math.round(Math.sqrt(Math.pow(this.vector.x, 2) + Math.pow(this.vector.y, 2)) * 1000) / 1000;
        }
    }
}

class Ruler
{
    constructor(x, y, colour="#0011FF")
    {
        this.point_1 = null;
        this.point_2 = null;
        this.line    = new Line(this.point_1, this.point_2);
        this.colour  = colour;
    }

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

class Mouse
{
    constructor(x, y)
    {
        this.x = x;
        this.y = y;
    }
}
