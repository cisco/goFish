class Canvas
{

}

class Point
{
    constructor(x, y, r)
    {
        this.x      = x;
        this.y      = y;
        this.radius = r;
    }
}

class Line
{
    constructor(p1, p2)
    {
        this.start_point = p1;
        this.end_point   = p2;
    }
}

class Ruler
{
    constructor(x, y)
    {
        this.point_1 = new Point(x, y);
        this.point_2 = new Point(-1, -1);
        this.line    = new Line(this.point_1, this.point_2);
    }


}