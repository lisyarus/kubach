#include "player.h"

#include <QtOpenGL>
#include <cmath>

const double player::size_x = 0.4;
const double player::size_y_top = 0.2;
const double player::size_y_bottom = 1.6;
const double player::size_z = 0.4;

void player::move (double step)
{
    double scale = step;
    if (move_forward != 0 && move_sideward != 0)
        scale /= sqrt(2.0);
    /*
    x += scale * move_forward * sin(alpha) * cos(beta);
    z -= scale * move_forward * cos(alpha) * cos(beta);

    x += scale * move_sideward * cos(alpha) * cos(beta);
    z += scale * move_sideward * sin(alpha) * cos(beta);

    y -= step * move_forward * sin(beta);
    */

    x += scale * move_forward * sin(alpha);
    z -= scale * move_forward * cos(alpha);

    x += scale * move_sideward * cos(alpha);
    z += scale * move_sideward * sin(alpha);

    y += step * move_upward;
    y += vy * step;

    double smooth = 0.99;
    _x += (x - _x) * step * smooth;
    _y += (y - _y) * step * smooth;
    _z += (z - _z) * step * smooth;
}

bool player::has_collision (const cube_position & c) const
{
    auto sqr = [](double x){ return x * x; };
    double tx = _x - c.x;
    double ty = _y - c.y;
    double tz = _z - c.z;

    if (sqr(tx) + sqr(ty) + sqr(tz) > 16.0) return false;

    double dx = fabs(tx);
    double dy = fabs(ty);
    double dz = fabs(tz);

    bool collision = (dx < 0.5 + size_x) && ((ty > 0 && ty < 0.5 + size_y_bottom) || (ty < 0 && ty > - 0.5 - size_y_top)) && (dz < 0.5 + size_z);

    return collision;
}

double player::distance (const cube & c) const
{
    return -1;
}

bool player::collide (const cube & c)
{
    double tx = _x - c.x;
    double ty = _y - c.y;
    double tz = _z - c.z;

    double dx = fabs(tx);
    double dy = fabs(ty);
    double dz = fabs(tz);

    bool collision = has_collision(c);

    bool on_surface = false;

    if (collision)
    {
        if (dx > dy && dx > dz)
        {
            if (tx > 0 && tx < 0.5 + size_x) x = c.x + 0.5 + size_x;
            if (tx < 0 && tx > - 0.5 - size_x) x = c.x - 0.5 - size_x;
            _x = x;
        }
        else if (dy > dz)
        {
            if (dx < 0.5 && dz < 0.5)
            {
                if (ty > 0 && ty < 0.5 + size_y_bottom)
                {
                    y = c.y + 0.5 + size_y_bottom;
                    on_surface = true;
                    vy = 0.0;
                }
                if (ty < 0 && ty > - 0.5 - size_y_top) y = c.y - 0.5 - size_y_top;
                _y = y;
            }
        }
        else
        {
            if (tz > 0 && tz < 0.5 + size_z) z = c.z + 0.5 + size_z;
            if (tz < 0 && tz > - 0.5 - size_z) z = c.z - 0.5 - size_z;
            _z = z;
        }
    }

    return on_surface;
}

void player::transform ( ) const
{
    rotate();
    translate();
}

void player::translate ( ) const
{
    glTranslated(-_x, -_y, -_z);
}

void player::rotate ( ) const
{
    glRotated(beta * 180.0 / 3.1415926535, 1.0, 0.0, 0.0);
    glRotated(alpha * 180.0 / 3.1415926535, 0.0, 1.0, 0.0);
}
