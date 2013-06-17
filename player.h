#ifndef PLAYER_H
#define PLAYER_H

#include "cube.h"

struct player
{
    double x, y, z;
    double vy;
    double alpha, beta;
    int move_forward, move_sideward, move_upward;

    const double size_x = 0.4;
    const double size_y_top = 0.2;
    const double size_y_bottom = 1.2;
    const double size_z = 0.4;

    player ( )
        : x(0.0), y(0.0), z(0.0),
        vy(0.0),
        alpha(0.0), beta(0.0),
        move_forward(0), move_sideward(0), move_upward(0)
    { }

    void move (double step);
    void translate ( ) const;
    void rotate ( ) const;
    void transform ( ) const;

    double distance (const cube & c) const;
    bool has_collision (const cube & c) const;
    bool collide (const cube & c);

    void init ( )
    {
        _x = x;
        _y = y;
        _z = z;
    }

private:
    double _x, _y, _z;
};

#endif // PLAYER_H
