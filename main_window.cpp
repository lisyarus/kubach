#include "main_window.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QTimer>

#include <random>
#include <functional>
#include <sstream>

#include <GL/glu.h>
#include <cmath>

main_window::main_window(QGLWidget *parent)
    : QGLWidget (parent)
{
    QApplication::setOverrideCursor(Qt::BlankCursor);
    setMouseTracking(true);

    setFixedSize(800, 600);

    brightness = 2.0;
    hue = 0.0;

    pl.y = 5.0;
    pl.vy = 1.5;
    pl.init();

    for (int x = -10; x <= 10; ++x)
        for (int z = -10; z <= 10; ++z)
            cubes.emplace_back(x, 0, z, get_current_color());

    for (int x = -10; x <= 10; ++x)
        for (int y = 1; y <= 10; ++y)
        {
            cubes.emplace_back(x, y, -10, get_current_color());
            cubes.emplace_back(x, y, 10, get_current_color());
        }

    for (int z = -9; z <= 9; ++z)
        for (int y = 1; y <= 10; ++y)
        {
            cubes.emplace_back(-10, y, z, get_current_color());
            cubes.emplace_back(10, y, z, get_current_color());
        }

    brightness = 1.0 + 0.5 / sphere_y;
    hue = -3.0 / sphere_x;

    has_chosen_plane = false;

    enable_gravity = true;
    on_surface = false;

    rainbow = false;

    last_frame = 0.0;
    startTimer(20);
}

main_window::~main_window()
{ }

void main_window::initializeGL ( )
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glDepthFunc(GL_LEQUAL);

    glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto random = std::bind(std::uniform_int_distribution<unsigned char>(240, 255), std::default_random_engine());
    for (int x = 0; x < texture_size; ++x)
    {
        for (int y = 0; y < texture_size; ++y)
        {
            unsigned char value = random();
            bool border = x == 0 || x == texture_size - 1 || y == 0 || y == texture_size - 1;

            texture[x * texture_size * 3 + y * 3 + 0] = border ? 0 : value;
            texture[x * texture_size * 3 + y * 3 + 1] = border ? 0 : value;
            texture[x * texture_size * 3 + y * 3 + 2] = border ? 0 : value;
        }
    }

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
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

color main_window::get_color (double brightness, double hue) const
{
    while (hue < 0.0) hue += 6.0;
    while (hue >= 6.0) hue -= 6.0;

    color res;
    if (hue >= 0.0 && hue < 1.0)
        res = color(1.0, hue, 0.0);
    else if (hue >= 1.0 && hue < 2.0)
        res = color(2.0 - hue, 1.0, 0.0);
    else if (hue >= 2.0 && hue < 3.0)
        res = color(0.0, 1.0, hue - 2.0);
    else if (hue >= 3.0 && hue < 4.0)
        res = color(0.0, 4.0 - hue, 1.0);
    else if (hue >= 4.0 && hue < 5.0)
        res = color(hue - 4.0, 0.0, 1.0);
    else if (hue >= 5.0 && hue < 6.0)
        res = color(1.0, 0.0, 6.0 - hue);

    if (brightness < 1.0)
    {
        res.data[0] *= brightness;
        res.data[1] *= brightness;
        res.data[2] *= brightness;
    }
    else
    {
        brightness -= 1.0;
        res.data[0] += (1.0 - res.data[0]) * brightness;
        res.data[1] += (1.0 - res.data[1]) * brightness;
        res.data[2] += (1.0 - res.data[2]) * brightness;
    }
    return res;
}

color main_window::get_current_color ( ) const
{
    color res = get_color(discrete_brightness(), discrete_hue());
    res.data[3] = 0.5;
    return res;
}

void main_window::set_color (color c) const
{
    glColor4dv(c.data);
}

int truncate (double x, int add)
{
    if (x >= 0)
        return x + add;
    else
        return x + add - 1;
}

double main_window::discrete_brightness ( ) const
{
    return truncate((brightness - 1.0) * sphere_y, 0) / static_cast<double>(sphere_y) + 1.0;
}

double main_window::discrete_hue ( ) const
{
    return truncate(hue * sphere_x / 6.0, 1) / static_cast<double>(sphere_x) * 6.0;
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

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    std::vector<double> vertices;
    std::vector<double> tex_coords;
    std::vector<double> colors;
    vertices.reserve(cubes.size() * 6 * 12);
    tex_coords.reserve(cubes.size() * 6 * 8);
    colors.reserve(cubes.size() * 6 * 16);

    for (size_t c = 0; c < cubes.size(); ++c)
    {
        for (int p = 0; p < 6; ++p)
        {
            for (int v = 0; v < 12; ++v)
                vertices.push_back(cubes[c].planes[p].coords[v]);
            for (int v = 0; v < 8; ++v)
                tex_coords.push_back(plane::tex_coords[v]);
            for (int v = 0; v < 4; ++v)
            {
                colors[c * 6 * 16 + p * 16 + v * 4 + 0] = cubes[c].c.data[0];
                colors[c * 6 * 16 + p * 16 + v * 4 + 1] = cubes[c].c.data[1];
                colors[c * 6 * 16 + p * 16 + v * 4 + 2] = cubes[c].c.data[2];
                colors[c * 6 * 16 + p * 16 + v * 4 + 3] = cubes[c].c.data[3];
            }
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_DOUBLE, 0, vertices.data());
    glTexCoordPointer(2, GL_DOUBLE, 0, tex_coords.data());
    glColorPointer(4, GL_DOUBLE, 0, colors.data());
    glDrawArrays(GL_QUADS, 0, cubes.size() * 6 * 4);

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

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    for (size_t c = 0; c < cubes.size(); ++c)
    {
        for (int p = 0; p < 6; ++p)
        {
            glLoadName(c * 6 + p);
            glDrawArrays(GL_QUADS, (c * 6 + p) * 4, 4);
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

    glDisable(GL_TEXTURE_2D);

    glColor3ub(255, 0, 0);
    for (const kubeman & k : kubemen)
        k.draw();

    glDisable(GL_DEPTH_TEST);

    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-ratio, ratio, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

    set_color(get_color(2.0 - brightness, hue + 3.0));
    glBegin(GL_LINES);
        glVertex2d(-cross_size, 0.0);
        glVertex2d(cross_size, 0.0);
        glVertex2d(0.0, -cross_size);
        glVertex2d(0.0, cross_size);
    glEnd();

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-ratio, ratio, -1.0, 1.0, -1.0, 10.0);
    //glFrustum(- ratio * 0.01, ratio * 0.01, -0.01, 0.01, 0.01, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    if (rainbow)
    {
        glScaled(0.8, 0.8, 0.8);
    }
    else
    {
        glScaled(0.2, 0.2, 0.2);
        glTranslated(0.0, -4.0, -5.0);
    }
    glPushMatrix();
    glRotated((brightness - 1.0) * 90, 1.0, 0.0, 0.0);

    for (int x = 0; x < sphere_x; ++x)
    {
        for (int y = -sphere_y; y < sphere_y; ++y)
        {
            double dx = 2.0 * 3.1415926535 / sphere_x;
            double dy = 0.5 * 3.1415926535 / sphere_y;
            double ax = (x - hue * sphere_x / 6.0 + sphere_x * 0.25) * dx;
            double ay = (y) * dy;
            glBegin(GL_QUADS);
                set_color(get_color(1.0 + static_cast<double>(y) / sphere_y, static_cast<double>(x + 1) / sphere_x * 6.0));
                glVertex3d(cos(ax) * cos(ay), sin(ay), sin(ax) * cos(ay));
                //set_color(get_color(1.0 + static_cast<double>(y) / sphere_y, static_cast<double>(x + 1) / sphere_x * 6.0));
                glVertex3d(cos(ax + dx) * cos(ay), sin(ay), sin(ax + dx) * cos(ay));
                //set_color(get_color(1.0 + static_cast<double>(y + 1) / sphere_y, static_cast<double>(x + 1) / sphere_x * 6.0));
                glVertex3d(cos(ax + dx) * cos(ay + dy), sin(ay + dy), sin(ax + dx) * cos(ay + dy));
                //set_color(get_color(1.0 + static_cast<double>(y + 1) / sphere_y, static_cast<double>(x) / sphere_x * 6.0));
                glVertex3d(cos(ax) * cos(ay + dy), sin(ay + dy), sin(ax) * cos(ay + dy));
            glEnd();
        }
    }
    glPopMatrix();

    if (rainbow)
    {
        glDisable(GL_DEPTH_TEST);
        set_color(get_current_color());
        glBegin(GL_TRIANGLES);
            glVertex3d(-0.1, -0.05, 1.1);
            glVertex3d(0.0, 0.0, 1.0);
            glVertex3d(0.1, -0.05, 1.1);
        glEnd();
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
            glVertex3d(-0.1, -0.05, 1.1);
            glVertex3d(0.0, 0.0, 1.0);
            glVertex3d(0.1, -0.05, 1.1);
        glEnd();
    }

    swapBuffers();

    auto now = std::chrono::high_resolution_clock::now();
    if (!frames.empty())
        last_frame = 0.001 * std::chrono::duration_cast<std::chrono::milliseconds>(now - frames.back()).count();

    frames.push(now);

    if (frames.size() > average_frames)
    {
        double fps = average_frames * 1000.0 / std::chrono::duration_cast<std::chrono::milliseconds>(now - frames.front()).count();
        frames.pop();
        
        std::ostringstream oss;
        oss << "Kubach. Try mouse buttons! FPS: ";
        oss << (int)fps;
        setWindowTitle(oss.str().c_str());
    }
}

void main_window::mouseMoveEvent (QMouseEvent * mouseEvent)
{
    static bool first = true;

    if (first)
        first = false;
    else if (rainbow)
    {
        QCursor::setPos(normalGeometry().topLeft() + QPoint(width / 2, height / 2));
        brightness -= (mouseEvent->y() - height / 2) * 0.0075;
        if (brightness > 2.0) brightness = 2.0;
        if (brightness < 0.0) brightness = 0.0;
        hue -= (mouseEvent->x() - width / 2) * 0.0075;
    }
    else
    {
        QCursor::setPos(normalGeometry().topLeft() + QPoint(width / 2, height / 2));
        pl.alpha += (mouseEvent->x() - width / 2) * 0.0075;
        pl.beta += (mouseEvent->y() - height / 2) * 0.0075;
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
            cubes.emplace_back(0, 0, 0, get_current_color());
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_R)
    {
        pl.x = 0.0;
        pl.y = 5.0;
        pl.z = 0.0;
        pl.vy = 1.5;
        pl.init();
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
    if (mouseEvent->button() == Qt::MouseButton::RightButton)
    {
        if (has_chosen_plane)
        {
            cube to_add = cubes[chosen_cube_index].planes[chosen_plane_index].adjacent_cube(get_current_color());
            if (!pl.has_collision(to_add))
                cubes.push_back(to_add);
        }
    }
    else if (mouseEvent->button() == Qt::MouseButton::LeftButton)
    {
        if (has_chosen_plane)
        {
            cubes.erase(cubes.begin() + chosen_cube_index);
        }
    }
    else if (mouseEvent->button() == Qt::MouseButton::MiddleButton)
    {
        rainbow = true;
    }
}

void main_window::mouseReleaseEvent (QMouseEvent * mouseEvent)
{
    if (mouseEvent->button() == Qt::MouseButton::MiddleButton)
    {
        rainbow = false;
    }
}

void main_window::wheelEvent (QWheelEvent * event)
{
    hue += event->delta() / 120.0 * 6.0 / sphere_x;
}

void main_window::timerEvent (QTimerEvent *)
{
    if (enable_gravity)
        pl.vy -= g * last_frame;
    pl.move(speed * last_frame);
    on_surface = false;
    for (const cube & c : cubes)
        on_surface |= pl.collide(c);
    updateGL();
}
