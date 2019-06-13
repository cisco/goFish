class Toolkit
{
    constructor()
    {
        this.canvas         = $('#adjusted-video');
        this.mouse          = null;
        this.left_rulers    = Array();
        this.right_rulers   = Array();
        this.lruler_index   = 0;
        this.rruler_index   = 0;
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
        if(this.mouse != null)
        {
            if( (this.mouse.x >= this.canvas.position().left && this.mouse.x <= this.canvas.position().left + this.canvas.width()) &&
                (this.mouse.y >= this.canvas.position().top && this.mouse.y <= this.canvas.position().top + this.canvas.height()))
            {
                console.log("Using tool");
                switch(this.mode)
                {
                    case 0:
                        this.AddRuler();
                        break;
                    case 1:
                        break;
                    case 2:
                        break;
                    case -1:
                    default:
                        console.log("No tool currently active!");
                }
            }
        }
    }

    AddRuler()
    {
        if(this.mouse.x <= this.canvas.position().left + this.canvas.width()/2)
        {
            if(this.left_rulers[this.lruler_index] == null) 
                this.left_rulers.push(new Ruler(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top, "#FF1144"));
            this.left_rulers[this.lruler_index].AddPoint(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top);
            if(this.left_rulers[this.lruler_index].point_2 != null)
            {
                var text = "<dt>" + this.lruler_index + ": </dt>";
                text += "<dd>x1 = " + this.left_rulers[this.lruler_index].point_1.x + "</dd>";
                text += "<dd>y1 = " + Math.round(this.left_rulers[this.lruler_index].point_1.y) + "</dd>";
                text += "<dd>x2 = " + this.left_rulers[this.lruler_index].point_2.x + "</dd>";
                text += "<dd>y2 = " + Math.round(this.left_rulers[this.lruler_index].point_2.y) + "</dd>";
                text += "<dd>Length = " + this.left_rulers[this.lruler_index].line.Length() + "</dd>";

                $("#ruler-info").append(text);

                var line = this.left_rulers[this.lruler_index].line;
                var data = {'point1' : {'x' : line.start_point.x, 'y':line.start_point.y}, 'point2' : {'x' : line.end_point.x, 'y':line.end_point.y}};
                var xhr = new XMLHttpRequest();
                xhr.open("POST", "/processing/", true);
                xhr.send(JSON.stringify(data));
                
                this.lruler_index++;
            }
        }
        else
        {
            if(this.right_rulers[this.rruler_index] == null) 
                this.right_rulers.push(new Ruler(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top, "#3333FF"));
            this.right_rulers[this.rruler_index].AddPoint(this.mouse.x - this.canvas.position().left, this.mouse.y - this.canvas.position().top);
            if(this.right_rulers[this.rruler_index].point_2 != null)
                this.rruler_index++;
        }
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

    Calculate3DPoints()
    {
        // <x, y, w> = P < X, Y, Z, 1>
        //
        //     | fx  0  px  0 |
        // P = |  0 fy  py  0 |
        //     |  0  0   1  0 |

        

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
