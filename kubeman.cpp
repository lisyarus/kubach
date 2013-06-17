#include "kubeman.h"
#include "player.h"

#include <QtOpenGL>

void kubeman::draw ( ) const
{
    glBegin(GL_LINES);
        glVertex3d(x, y - player::size_y_bottom, z);
        glVertex3d(x, y + player::size_y_top, z);
    glEnd();
}
