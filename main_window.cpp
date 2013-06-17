#include "main_window.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QTimer>

#include <random>
#include <functional>
#include <sstream>

#include <GL/glu.h>

main_window::main_window(QGLWidget *parent)
    : QGLWidget (parent)
{
    QApplication::setOverrideCursor(Qt::BlankCursor);
    setMouseTracking(true);

    pl.y = 0.0;
    pl.z = 5.0;
    pl.init();

    cubes.emplace_back(0, 0, 0);

    has_chosen_plane = false;

    show_grid = false;

    enable_gravity = false;
    on_surface = false;

    startTimer(20);
}

main_window::~main_window()
{ }

void main_window::initializeGL ( )
{
    glClearColor(0.0, 0.0, 0.25, 1.0);
    glDepthFunc(GL_LEQUAL);

    glGenTextures(1, &texture_id);
    glGenTextures(1, &choose_texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, choose_texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    auto random = std::bind(std::uniform_int_distribution<unsigned char>(127, 255), std::default_random_engine());
    for (int x = 0; x < texture_size; ++x)
    {
        for (int y = 0; y < texture_size; ++y)
        {
            unsigned char value = random();
            bool border = x == 0 || x == texture_size - 1 || y == 0 || y == texture_size - 1;

            texture[x * texture_size * 3 + y * 3 + 0] = border ? 0 : value;
            texture[x * texture_size * 3 + y * 3 + 1] = border ? 0 : value;
            texture[x * texture_size * 3 + y * 3 + 2] = border ? 0 : value;

            choose_texture[x * texture_size * 3 + y * 3 + 0] = border ? 0 : value;
            choose_texture[x * texture_size * 3 + y * 3 + 1] = border ? 255 : value;
            choose_texture[x * texture_size * 3 + y * 3 + 2] = border ? 0 : value;
        }
    }

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

    glBindTexture(GL_TEXTURE_2D, choose_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, choose_texture);
}

void main_window::resizeGL (int width, int height)
{
    this->width = width;
    this->height = height;

    QCursor::setPos(normalGeometry().topLeft() + QPoint(width / 2, height / 2));

    pl.alpha = 0.0;
    pl.beta = 0.0;

    ratio = static_cast<double>(width) / height;

    glViewport(0, 0, width, height);
}

void main_window::paintGL ( )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(- ratio * 0.01, ratio * 0.01, -0.01, 0.01, 0.01, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    pl.transform();


    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0, 1.0, 1.0);
    if (show_grid)
    {
        glBegin(GL_LINES);
            for (int i = -10; i <= 10; ++i)
            {
                glVertex3d(i, 0.0, -10.0);
                glVertex3d(i, 0.0, 10.0);

                glVertex3d(-10.0, 0.0, i);
                glVertex3d(10.0, 0.0, i);
            }
        glEnd();
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    for (size_t c = 0; c < cubes.size(); ++c)
    {
        for (int p = 0; p < 6; ++p)
        {
            bool chosen = has_chosen_plane && chosen_cube_index == c && chosen_plane_index == p;
            if (chosen)
                glBindTexture(GL_TEXTURE_2D, choose_texture_id);
            cubes[c].planes[p].draw();
            if (chosen)
                glBindTexture(GL_TEXTURE_2D, texture_id);
        }
    }

    unsigned int buffer[512];
    unsigned int hits;
    glSelectBuffer(512, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPickMatrix(width * 0.5, height * 0.5, 1.0, 1.0, viewport);
    gluPerspective(45.0f, (GLfloat) (viewport[2]-viewport[0])/(GLfloat) (viewport[3]-viewport[1]), 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);

    for (size_t c = 0; c < cubes.size(); ++c)
    {
        for (int p = 0; p < 6; ++p)
        {
            glLoadName(c * 6 + p);
            cubes[c].planes[p].draw();
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    hits = glRenderMode(GL_RENDER);
    if (hits > 0)
    {
        int min = 0;

        for (size_t i = 0; i < hits; ++i)
        {
            if (buffer[i * 4 + 1] < buffer[min * 4 + 1])
            {
                min = i;
            }
        }

        size_t index = buffer[min * 4 + 3];
        chosen_cube_index = index / 6;
        chosen_plane_index = index % 6;

        //qDebug("%ui", buffer[min * 4 + 1]);

        if (index >= 0 && index < cubes.size() * 6)
        {
            has_chosen_plane = true;
        }
        else
            has_chosen_plane = false;
    }
    else
    {
        has_chosen_plane = false;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-ratio, ratio, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

    glBegin(GL_LINES);
        glVertex2d(-cross_size, 0.0);
        glVertex2d(cross_size, 0.0);
        glVertex2d(0.0, -cross_size);
        glVertex2d(0.0, cross_size);
    glEnd();

    swapBuffers();

    auto now = std::chrono::high_resolution_clock::now();
    frames.push(now);

    if (frames.size() >= 5)
    {
        double fps = 5000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(now - frames.front()).count();
        frames.pop();
        
        std::ostringstream oss;
        oss << (int)fps;
        setWindowTitle(oss.str().c_str());
    }
}

void main_window::mouseMoveEvent (QMouseEvent * mouseEvent)
{
    static bool first = true;

    if (first)
        first = false;
    else
    {
        QCursor::setPos(normalGeometry().topLeft() + QPoint(width / 2, height / 2));
        pl.alpha += (mouseEvent->x() - width / 2) * speed * 0.05;
        pl.beta += (mouseEvent->y() - height / 2) * speed * 0.05;
        if (pl.beta > 3.1415926535 * 0.5) pl.beta = 3.1415926535 * 0.5;
        if (pl.beta < - 3.1415926535 * 0.5) pl.beta = - 3.1415926535 * 0.5;
    }
}

void main_window::keyPressEvent (QKeyEvent * keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape)
    {
        releaseMouse();
        QApplication::restoreOverrideCursor();
        QApplication::quit();
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_W)
    {
        pl.move_forward = 1;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_S)
    {
        pl.move_forward = -1;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_D)
    {
        pl.move_sideward = 1;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_A)
    {
        pl.move_sideward = -1;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_Space)
    {
        if (!enable_gravity)
            pl.move_upward = 1;
        else
            if (on_surface)
                pl.vy = 1.5;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_Shift)
    {
        if (!enable_gravity)
            pl.move_upward = -1;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_G)
    {
        enable_gravity ^= true;
        if (!enable_gravity)
            pl.vy = 0.0;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_O)
    {
        bool found = false;
        for (cube const & c : cubes)
            if (c.x == 0 && c.y == 0 && c.z == 0)
            {
                found = true;
                break;
            }
        if (!found)
            cubes.emplace_back(0, 0, 0);
        keyEvent->accept();
    }
}

void main_window::keyReleaseEvent (QKeyEvent * keyEvent)
{
    if (keyEvent->key() == Qt::Key_W)
    {
        pl.move_forward = 0;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_S)
    {
        pl.move_forward = 0;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_D)
    {
        pl.move_sideward = 0;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_A)
    {
        pl.move_sideward = 0;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_Space)
    {
        pl.move_upward = 0;
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_Shift)
    {
        pl.move_upward = 0;
        keyEvent->accept();
    }
}

void main_window::mousePressEvent (QMouseEvent * mouseEvent)
{
    if (mouseEvent->button() == Qt::MouseButton::LeftButton)
    {
        if (has_chosen_plane)
        {
            cube to_add = cubes[chosen_cube_index].planes[chosen_plane_index].adjacent_cube();
            if (!pl.has_collision(to_add))
                cubes.push_back(to_add);
        }
    }
    else if (mouseEvent->button() == Qt::MouseButton::RightButton)
    {
        if (has_chosen_plane)
        {
            cubes.erase(cubes.begin() + chosen_cube_index);
        }
    }
}

void main_window::timerEvent (QTimerEvent *)
{
    if (enable_gravity)
        pl.vy -= g * 0.1;
    pl.move(speed);
    on_surface = false;
    for (const cube & c : cubes)
        on_surface |= pl.collide(c);
    updateGL();
}
