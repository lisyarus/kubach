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

struct cube_position
{
    int x, y, z;

    cube_position ( ) = default;
    cube_position (int x, int y, int z)
        : x(x), y(y), z(z)
    { }
};

inline bool operator < (cube_position const & cp1, cube_position const & cp2)
{
    return (cp1.x < cp2.x) || (cp1.x == cp2.x && cp1.y < cp2.y) || (cp1.x == cp2.x && cp1.y == cp2.y && cp1.z < cp2.z);
}

struct plane
{
    static const double tex_coords[8];

    double hue, brightness;

    double coords[12];

    int cx, cy, cz, dx, dy, dz;
    cube_position adjacent_cube ( ) const;
};

bool operator == (const plane & p1, const plane & p2);

struct cube
    : cube_position
{
    plane planes[6];
};

cube colored_cube (cube_position, double hue, double brightness);

#endif // CUBE_H
