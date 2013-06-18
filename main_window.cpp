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

    pl.y = 5.0;
    pl.init();

    next_rainbow = [](){
        static auto gen = std::bind(std::uniform_int_distribution<int>(0, 8), std::default_random_engine());
        return gen();
    };

    for (int x = -10; x <= 10; ++x)
        for (int z = -10; z <= 10; ++z)
            cubes.emplace_back(x, 0, z, rainbows[next_rainbow()]);

    for (int x = -10; x <= 10; ++x)
        for (int y = 1; y <= 10; ++y)
        {
            cubes.emplace_back(x, y, -10, rainbows[next_rainbow()]);
            cubes.emplace_back(x, y, 10, rainbows[next_rainbow()]);
        }

    for (int z = -9; z <= 9; ++z)
        for (int y = 1; y <= 10; ++y)
        {
            cubes.emplace_back(-10, y, z, rainbows[next_rainbow()]);
            cubes.emplace_back(10, y, z, rainbows[next_rainbow()]);
        }

    has_chosen_plane = false;

    show_grid = false;

    enable_gravity = true;
    on_surface = false;

    last_frame = 0.0;
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
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, choose_texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto safe_add = [] (unsigned char c, unsigned char a) -> unsigned char
    {
        unsigned int t = static_cast<unsigned int>(c) + a;
        if (t > 255) t = 255;
        return static_cast<unsigned char>(t);
    };

    auto random = std::bind(std::uniform_int_distribution<unsigned char>(192, 255), std::default_random_engine());
    for (int x = 0; x < texture_size; ++x)
    {
        for (int y = 0; y < texture_size; ++y)
        {
            unsigned char value = random();
            bool border = x == 0 || x == texture_size - 1 || y == 0 || y == texture_size - 1;

            texture[x * texture_size * 3 + y * 3 + 0] = border ? 0 : value;
            texture[x * texture_size * 3 + y * 3 + 1] = border ? 0 : value;
            texture[x * texture_size * 3 + y * 3 + 2] = border ? 0 : safe_add(value, 32);

            choose_texture[x * texture_size * 3 + y * 3 + 0] = border ? 0 : value;
            choose_texture[x * texture_size * 3 + y * 3 + 1] = border ? 255 : value;
            choose_texture[x * texture_size * 3 + y * 3 + 2] = border ? 0 : safe_add(value, 32);
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
                colors[c * 6 * 16 + p * 16 + v * 4 + 0] = cubes[c].r;
                colors[c * 6 * 16 + p * 16 + v * 4 + 1] = cubes[c].g;
                colors[c * 6 * 16 + p * 16 + v * 4 + 2] = cubes[c].b;
                colors[c * 6 * 16 + p * 16 + v * 4 + 3] = 0.5;
            }
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    int chosen_index = chosen_cube_index * 6 + chosen_plane_index;

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

    glColor3ub(255, 255, 255);
    glBegin(GL_LINES);
        glVertex2d(-cross_size, 0.0);
        glVertex2d(cross_size, 0.0);
        glVertex2d(0.0, -cross_size);
        glVertex2d(0.0, cross_size);
    glEnd();

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
            cubes.emplace_back(0, 0, 0, rainbows[next_rainbow()]);
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
    if (mouseEvent->button() == Qt::MouseButton::RightButton)
    {
        if (has_chosen_plane)
        {
            cube to_add = cubes[chosen_cube_index].planes[chosen_plane_index].adjacent_cube(rainbows[next_rainbow()]);
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
