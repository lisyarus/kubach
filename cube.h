#ifndef CUBE_H
#define CUBE_H

struct cube;

struct plane
{
    static const double tex_coords[8];

    double coords[12];

    void draw ( ) const;

    int cx, cy, cz, dx, dy, dz;
    cube adjacent_cube (const double* colors) const;
};

bool operator == (const plane & p1, const plane & p2);

struct cube
{
    cube (int x, int y, int z, const double* colors);

    void draw ( ) const;

    plane planes[6];

    int x, y, z;

    double r, g, b;
};

#endif // CUBE_H
