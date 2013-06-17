#pragma once

struct kubeman
{
    double x, y, z;

    kubeman ( ) { }
    kubeman (double x, double y, double z)
        : x(x), y(y), z(z)
    { }

    void draw ( ) const;
};
