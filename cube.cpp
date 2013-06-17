#include "cube.h"

#include <QtOpenGL>

const double plane::tex_coords[8] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0};

bool operator == (const plane & p1, const plane & p2)
{
    return p1.cx == p2.cx && p1.cy == p2.cy && p1.cz == p2.cz
        && p1.dx == p2.dx && p1.dy == p2.dy && p1.dz == p2.dz;
}

void plane::draw ( ) const
{
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; ++i)
    {
        glTexCoord2dv(tex_coords + i * 2);
        glVertex3dv(coords + i * 3);
    }
    glEnd();
}

cube plane::adjacent_cube ( ) const
{
    return cube(cx + dx, cy + dy, cz + dz);
}

cube::cube (int x, int y, int z)
    : x(x), y(y), z(z)
{
    for (int p = 0; p < 6; ++p)
    {
        planes[p].cx = x;
        planes[p].cy = y;
        planes[p].cz = z;

        planes[p].dx = 0;
        planes[p].dy = 0;
        planes[p].dz = 0;

        for (int i = 0; i < 12; i += 3)
        {
            planes[p].coords[i + 0] = x;
            planes[p].coords[i + 1] = y;
            planes[p].coords[i + 2] = z;
        }
    }

    // +x

    planes[0].dx = 1;

    planes[0].coords[ 0] += 0.5;
    planes[0].coords[ 3] += 0.5;
    planes[0].coords[ 6] += 0.5;
    planes[0].coords[ 9] += 0.5;
    planes[0].coords[ 1] -= 0.5;
    planes[0].coords[ 4] += 0.5;
    planes[0].coords[ 7] += 0.5;
    planes[0].coords[10] -= 0.5;
    planes[0].coords[ 2] -= 0.5;
    planes[0].coords[ 5] -= 0.5;
    planes[0].coords[ 8] += 0.5;
    planes[0].coords[11] += 0.5;

    // -x

    planes[1].dx = -1;

    planes[1].coords[ 0] -= 0.5;
    planes[1].coords[ 3] -= 0.5;
    planes[1].coords[ 6] -= 0.5;
    planes[1].coords[ 9] -= 0.5;
    planes[1].coords[ 1] -= 0.5;
    planes[1].coords[ 4] += 0.5;
    planes[1].coords[ 7] += 0.5;
    planes[1].coords[10] -= 0.5;
    planes[1].coords[ 2] -= 0.5;
    planes[1].coords[ 5] -= 0.5;
    planes[1].coords[ 8] += 0.5;
    planes[1].coords[11] += 0.5;

    // +y

    planes[2].dy = 1;

    planes[2].coords[ 0] -= 0.5;
    planes[2].coords[ 3] += 0.5;
    planes[2].coords[ 6] += 0.5;
    planes[2].coords[ 9] -= 0.5;
    planes[2].coords[ 1] += 0.5;
    planes[2].coords[ 4] += 0.5;
    planes[2].coords[ 7] += 0.5;
    planes[2].coords[10] += 0.5;
    planes[2].coords[ 2] -= 0.5;
    planes[2].coords[ 5] -= 0.5;
    planes[2].coords[ 8] += 0.5;
    planes[2].coords[11] += 0.5;

    // -y

    planes[3].dy = -1;

    planes[3].coords[ 0] -= 0.5;
    planes[3].coords[ 3] += 0.5;
    planes[3].coords[ 6] += 0.5;
    planes[3].coords[ 9] -= 0.5;
    planes[3].coords[ 1] -= 0.5;
    planes[3].coords[ 4] -= 0.5;
    planes[3].coords[ 7] -= 0.5;
    planes[3].coords[10] -= 0.5;
    planes[3].coords[ 2] -= 0.5;
    planes[3].coords[ 5] -= 0.5;
    planes[3].coords[ 8] += 0.5;
    planes[3].coords[11] += 0.5;

    // +z

    planes[4].dz = 1;

    planes[4].coords[ 0] -= 0.5;
    planes[4].coords[ 3] += 0.5;
    planes[4].coords[ 6] += 0.5;
    planes[4].coords[ 9] -= 0.5;
    planes[4].coords[ 1] -= 0.5;
    planes[4].coords[ 4] -= 0.5;
    planes[4].coords[ 7] += 0.5;
    planes[4].coords[10] += 0.5;
    planes[4].coords[ 2] += 0.5;
    planes[4].coords[ 5] += 0.5;
    planes[4].coords[ 8] += 0.5;
    planes[4].coords[11] += 0.5;

    // -z

    planes[5].dz = -1;

    planes[5].coords[ 0] -= 0.5;
    planes[5].coords[ 3] += 0.5;
    planes[5].coords[ 6] += 0.5;
    planes[5].coords[ 9] -= 0.5;
    planes[5].coords[ 1] -= 0.5;
    planes[5].coords[ 4] -= 0.5;
    planes[5].coords[ 7] += 0.5;
    planes[5].coords[10] += 0.5;
    planes[5].coords[ 2] -= 0.5;
    planes[5].coords[ 5] -= 0.5;
    planes[5].coords[ 8] -= 0.5;
    planes[5].coords[11] -= 0.5;
}

void cube::draw ( ) const
{
    for (int p = 0; p < 6; ++p)
        planes[p].draw();
}
