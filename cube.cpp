#include "cube.h"

#include <QtOpenGL>

const double plane::tex_coords[8] = {0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0};

bool operator == (const plane & p1, const plane & p2)
{
    return p1.cx == p2.cx && p1.cy == p2.cy && p1.cz == p2.cz
        && p1.dx == p2.dx && p1.dy == p2.dy && p1.dz == p2.dz;
}

cube_position plane::adjacent_cube ( ) const
{
    return cube_position(cx + dx, cy + dy, cz + dz);
}

cube make_mesh (cube_position pos)
{
    cube result;
    reinterpret_cast<cube_position &>(result) = pos;

    for (int p = 0; p < 6; ++p)
    {
        result.planes[p].cx = pos.x;
        result.planes[p].cy = pos.y;
        result.planes[p].cz = pos.z;

        result.planes[p].dx = 0;
        result.planes[p].dy = 0;
        result.planes[p].dz = 0;

        for (int i = 0; i < 12; i += 3)
        {
            result.planes[p].coords[i + 0] = pos.x;
            result.planes[p].coords[i + 1] = pos.y;
            result.planes[p].coords[i + 2] = pos.z;
        }
    }

    // +x

    result.planes[0].dx = 1;

    result.planes[0].coords[ 0] += 0.5;
    result.planes[0].coords[ 3] += 0.5;
    result.planes[0].coords[ 6] += 0.5;
    result.planes[0].coords[ 9] += 0.5;
    result.planes[0].coords[ 1] -= 0.5;
    result.planes[0].coords[ 4] += 0.5;
    result.planes[0].coords[ 7] += 0.5;
    result.planes[0].coords[10] -= 0.5;
    result.planes[0].coords[ 2] -= 0.5;
    result.planes[0].coords[ 5] -= 0.5;
    result.planes[0].coords[ 8] += 0.5;
    result.planes[0].coords[11] += 0.5;

    // -x

    result.planes[1].dx = -1;

    result.planes[1].coords[ 0] -= 0.5;
    result.planes[1].coords[ 3] -= 0.5;
    result.planes[1].coords[ 6] -= 0.5;
    result.planes[1].coords[ 9] -= 0.5;
    result.planes[1].coords[ 1] -= 0.5;
    result.planes[1].coords[ 4] += 0.5;
    result.planes[1].coords[ 7] += 0.5;
    result.planes[1].coords[10] -= 0.5;
    result.planes[1].coords[ 2] += 0.5;
    result.planes[1].coords[ 5] += 0.5;
    result.planes[1].coords[ 8] -= 0.5;
    result.planes[1].coords[11] -= 0.5;

    // +y

    result.planes[2].dy = 1;

    result.planes[2].coords[ 0] -= 0.5;
    result.planes[2].coords[ 3] += 0.5;
    result.planes[2].coords[ 6] += 0.5;
    result.planes[2].coords[ 9] -= 0.5;
    result.planes[2].coords[ 1] += 0.5;
    result.planes[2].coords[ 4] += 0.5;
    result.planes[2].coords[ 7] += 0.5;
    result.planes[2].coords[10] += 0.5;
    result.planes[2].coords[ 2] += 0.5;
    result.planes[2].coords[ 5] += 0.5;
    result.planes[2].coords[ 8] -= 0.5;
    result.planes[2].coords[11] -= 0.5;

    // -y

    result.planes[3].dy = -1;

    result.planes[3].coords[ 0] -= 0.5;
    result.planes[3].coords[ 3] += 0.5;
    result.planes[3].coords[ 6] += 0.5;
    result.planes[3].coords[ 9] -= 0.5;
    result.planes[3].coords[ 1] -= 0.5;
    result.planes[3].coords[ 4] -= 0.5;
    result.planes[3].coords[ 7] -= 0.5;
    result.planes[3].coords[10] -= 0.5;
    result.planes[3].coords[ 2] -= 0.5;
    result.planes[3].coords[ 5] -= 0.5;
    result.planes[3].coords[ 8] += 0.5;
    result.planes[3].coords[11] += 0.5;

    // +z

    result.planes[4].dz = 1;

    result.planes[4].coords[ 0] -= 0.5;
    result.planes[4].coords[ 3] += 0.5;
    result.planes[4].coords[ 6] += 0.5;
    result.planes[4].coords[ 9] -= 0.5;
    result.planes[4].coords[ 1] -= 0.5;
    result.planes[4].coords[ 4] -= 0.5;
    result.planes[4].coords[ 7] += 0.5;
    result.planes[4].coords[10] += 0.5;
    result.planes[4].coords[ 2] += 0.5;
    result.planes[4].coords[ 5] += 0.5;
    result.planes[4].coords[ 8] += 0.5;
    result.planes[4].coords[11] += 0.5;

    // -z

    result.planes[5].dz = -1;

    result.planes[5].coords[ 0] -= 0.5;
    result.planes[5].coords[ 3] += 0.5;
    result.planes[5].coords[ 6] += 0.5;
    result.planes[5].coords[ 9] -= 0.5;
    result.planes[5].coords[ 1] += 0.5;
    result.planes[5].coords[ 4] += 0.5;
    result.planes[5].coords[ 7] -= 0.5;
    result.planes[5].coords[10] -= 0.5;
    result.planes[5].coords[ 2] -= 0.5;
    result.planes[5].coords[ 5] -= 0.5;
    result.planes[5].coords[ 8] -= 0.5;
    result.planes[5].coords[11] -= 0.5;

    return result;
}

cube colored_cube (cube_position pos, color c)
{
    cube result = make_mesh(pos);
    for (int p = 0; p < 6; ++p)
    {
        for (int ci = 0; ci < 4; ++ci)
            result.planes[p].c[ci] = c;
    }
    return result;
}
