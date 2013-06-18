#ifndef CUBE_H
#define CUBE_H

struct color
{
    double data[4];

    color (double r, double g, double b, double a = 1.0)
        : data({r, g, b, a})
    { }

    color ( ) { }
};

struct cube;

struct plane
{
    static const double tex_coords[8];

    double coords[12];

    void draw ( ) const;

    int cx, cy, cz, dx, dy, dz;
    cube adjacent_cube (color c) const;
};

bool operator == (const plane & p1, const plane & p2);

struct cube
{
    cube (int x, int y, int z, color c);

    void draw ( ) const;

    plane planes[6];

    int x, y, z;

    color c;
};

#endif // CUBE_H
