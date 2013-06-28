#include "main_window.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QTimer>

#include <random>
#include <functional>
#include <sstream>
#include <iostream>

#include <GL/gl.h>
#include <GL/glu.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/glext.h>

#include <cmath>

main_window::main_window(QGLWidget *parent)
    : QGLWidget (parent)
{
    QApplication::setOverrideCursor(Qt::BlankCursor);
    setMouseTracking(true);

    setFixedSize(800, 600);

    auto randomc = std::bind(std::uniform_int_distribution<int>(0, world_size - 1), std::default_random_engine());
    auto randomh = std::bind(std::uniform_int_distribution<int>(-3, 3), std::default_random_engine());


    randomf = std::bind(std::uniform_real_distribution<double>(0.0, 1.0), std::default_random_engine());

    int start = (-1) << 0;

    std::vector<std::vector<int> > height(world_size, std::vector<int>(world_size, start + 5));

    for (int iter = 0; iter < world_size * world_size / 10; ++iter)
    {
        int x = randomc();
        int z = randomc();
        int h = randomh();
        if (h == 0) continue;

        int ah = (h > 0) ? h : -h;
        int th = (h > 0) ? 1 : -1;

        for (int dx = -ah; dx <= ah; ++dx)
            for (int dz = -ah; dz <= ah; ++dz)
                if (x + dx >= 0 && x + dx < world_size && z + dz >= 0 && z + dz < world_size)
                {
                    height[x + dx][z + dz] += th * (ah - std::max(abs(dx), abs(dz)));
                }
    }

    hue = 0.0;
    brightness = 0.4;

    for (int x = 0; x < world_size; ++x)
        for (int z = 0; z < world_size; ++z)
        {
            for (int y = start; y < height[x][z]; ++y)
                add_cube(x, y, z);

            add_cube(x, height[x][z], z);
            for (int ci = 0; ci < 4; ++ci)
            {
                cubes.back().planes[2].hue = 1.5;
                cubes.back().planes[2].brightness = 0.7;
            }
        }

    brightness = 1.0 + 0.5 / sphere_y;
    hue = -3.0 / sphere_x;

    sphere_hue = hue;
    sphere_brightness = brightness;

    pl.x = world_size * 0.5;
    pl.z = world_size * 0.5;
    pl.y = start + 100.0;
    pl.vy = 0.0;
    pl.init();

    has_chosen_plane = false;

    enable_gravity = false;
    on_surface = false;

    rainbow = false;

    health = 1.0;

    last_frame = 0.0;
    startTimer(10);
}

main_window::~main_window()
{ }

void main_window::initializeGL ( )
{
    glDepthFunc(GL_LEQUAL);

    glGenTextures(1, &texture_id);

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    auto random = std::bind(std::uniform_int_distribution<unsigned char>(192, 255), std::default_random_engine());
    for (int x = 0; x < texture_size; ++x)
    {
        for (int y = 0; y < texture_size; ++y)
        {
            unsigned char value = random();
            bool border = x == 0 || x == texture_size - 1 || y == 0 || y == texture_size - 1;
            int border_value = 127;

            texture[x * texture_size * 3 + y * 3 + 0] = border ? border_value : value;
            texture[x * texture_size * 3 + y * 3 + 1] = border ? border_value : value;
            texture[x * texture_size * 3 + y * 3 + 2] = border ? border_value : value;
        }
    }

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, texture_size, texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);

    const char * vertex_shader_code = "\
    uniform float health; \
    uniform vec4 relocate; \
    void main() { \
        gl_Position = gl_ModelViewProjectionMatrix * (gl_Vertex + relocate); \
        gl_FrontColor = gl_Color * (1.0 - health); \
        gl_FrontColor[0] = gl_Color[0] + health * (1.0 - gl_Color[0]); \
        gl_FrontColor[3] = gl_Color[3]; \
        gl_BackColor = gl_Color; \
         \
    }";
    const char * fragment_shader_code = "\
    uniform sampler2D texture1; \
    void main() { \
        gl_FragColor = gl_Color; \
    }";
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertex_shader, 1, &vertex_shader_code, 0);
    glShaderSource(fragment_shader, 1, &fragment_shader_code, 0);

    glCompileShader(vertex_shader);
    glCompileShader(fragment_shader);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    glUseProgram(program);

    uniform_satan = glGetUniformLocation(program, "health");
    relocate_addr = glGetUniformLocation(program, "relocate");
    std::cerr << "Health: " << uniform_satan << "\nRelocate: " << relocate_addr << '\n';
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
    res.data[3] = 1.0;
    return res;
}

color main_window::get_current_color ( ) const
{
    color res = get_color(discrete_brightness(), discrete_hue());
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

void main_window::add_cube (int x, int y, int z)
{
    cubes.push_back(colored_cube(cube_position(x, y, z), discrete_hue(), discrete_brightness()));
    cube_positions.insert(cube_position(x, y, z));
}

void main_window::paintGL ( )
{
    float dispersion = 1.0 - health;
    glClearColor(0.7 + 0.3 * dispersion, 0.8 * health, 1.0 * health, 1.0 * health);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    /*glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);*/

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(- ratio * 0.01, ratio * 0.01, -0.01, 0.01, 0.01, 1000.0);
    //glOrtho(-10.0, 10.0, -10.0, 10.0, -100.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    pl.transform();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    std::vector<double> vertices;
    std::vector<double> tex_coords;
    std::vector<double> colors;
    std::vector<double> relocations;
    vertices.reserve(cubes.size() * 6 * 12);
    tex_coords.reserve(cubes.size() * 6 * 8);
    colors.reserve(cubes.size() * 6 * 16);
    relocations.reserve(cubes.size() * 6 * 16);

    std::vector<int> indices;
    indices.reserve(cubes.size() * 6);

    glUniform1f(uniform_satan, dispersion);
    double h = health;
    std::function<double()> r = randomf;
    auto random_earthquake = [h, r](){ return 0.1 * (1.0 - h) * (2.0 * r() - 1.0); };
    glUniform4f(relocate_addr, random_earthquake(), random_earthquake(), random_earthquake(), 0.0);
    const float q = 0.07;
    { float health = 0.0;
    float v3[3] = {
        randomf() * (1.0 - health) * q,
        randomf() * (1.0 - health) * q,
        randomf() * (1.0 - health) * q
    };

    int count = 0;
    for (size_t c = 0; c < cubes.size(); ++c)
    {
        for (int p = 0; p < 6; ++p)
        {
            double rx = pl._x - cubes[c].planes[p].cx - cubes[c].planes[p].dx * 0.5;
            double ry = pl._y - cubes[c].planes[p].cy - cubes[c].planes[p].dy * 0.5;
            double rz = pl._z - cubes[c].planes[p].cz - cubes[c].planes[p].dz * 0.5;

            double r = rx * cubes[c].planes[p].dx + ry * cubes[c].planes[p].dy + rz * cubes[c].planes[p].dz;

            if (r < 0) continue;

            if (cube_positions.find(cube_position(cubes[c].planes[p].cx + cubes[c].planes[p].dx,
                                                  cubes[c].planes[p].cy + cubes[c].planes[p].dy,
                                                  cubes[c].planes[p].cz + cubes[c].planes[p].dz)) != cube_positions.end())
                continue;

            indices.push_back(c * 6 + p);
            float z = randomf() * q * std::sin(this->pl.vy);
            for (int v = 0; v < 4; ++v)
            {
                vertices.push_back(cubes[c].planes[p].coords[3 * v + 0]);
                vertices.push_back(cubes[c].planes[p].coords[3 * v + 1]);
                vertices.push_back(cubes[c].planes[p].coords[3 * v + 2]);

                if (health < 0.999f) {
                    relocations.push_back(z);
                    relocations.push_back(z);
                    relocations.push_back(0.0f);
                    relocations.push_back(0.0f);
                } else {
                    relocations.push_back(0.0f);
                    relocations.push_back(0.0f);
                    relocations.push_back(0.0f);
                    relocations.push_back(0.0f);
                }
            }
            

            for (int v = 0; v < 8; ++v)
                tex_coords.push_back(plane::tex_coords[v]);

            color col = get_color(cubes[c].planes[p].brightness, cubes[c].planes[p].hue);
            for (int v = 0; v < 4; ++v)
            {
                for (int ci = 0; ci < 4; ++ci)
                {
                    colors.push_back(col.data[ci]);
                }
            }
        }
    }}

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_DOUBLE, 0, vertices.data());
    glTexCoordPointer(2, GL_DOUBLE, 0, tex_coords.data());
    glColorPointer(4, GL_DOUBLE, 0, colors.data());
    //glVertexAttribPointer(relocate_addr, 4, GL_DOUBLE, GL_FALSE, 0, relocations.data());

    /*unsigned int buffer[512];
    int hits;
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

    for (int i = 0; i < indices.size(); ++i)
    {
        auto sqr = [](double x){ return x * x; };
        int cube_index = indices[i] / 6;
        if (sqr(pl.x - cubes[cube_index].x) + sqr(pl.y - cubes[cube_index].y) + sqr(pl.z - cubes[cube_index].z) < 25)
        {
            glLoadName(indices[i]);
            glDrawArrays(GL_QUADS, i * 4, 4);
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    hits = glRenderMode(GL_RENDER);
    if (hits > 0)
    {
        //qDebug("%i", hits);

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

    if (has_chosen_plane)
    for (int i = 0; i < indices.size(); ++i)
    {
        if (indices[i] == chosen_cube_index * 6 + chosen_plane_index)
        {
            for (int v = 0; v < 4; ++v)
            {
                colors[16 * i + v * 4 + 0] += 0.2;
                colors[16 * i + v * 4 + 1] += 0.2;
                colors[16 * i + v * 4 + 2] += 0.2;
            }
        }
    }*/

    glDrawArrays(GL_QUADS, 0, indices.size() * 4);

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
        glTranslated(-5.0, -4.0, -5.0);
    }
    glPushMatrix();
    glRotated((sphere_brightness - 1.0) * 90, 1.0, 0.0, 0.0);

    for (int x = 0; x < sphere_x; ++x)
    {
        for (int y = -sphere_y; y < sphere_y; ++y)
        {
            double dx = 2.0 * 3.1415926535 / sphere_x;
            double dy = 0.5 * 3.1415926535 / sphere_y;
            double ax = (x - sphere_hue * sphere_x / 6.0 + sphere_x * 0.25) * dx;
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

    /*glColor4d(1.0, 0.0, 0.0, 1.0 - health);
    glBegin(GL_QUADS);
        glVertex3d(-100, -100, 10);
        glVertex3d(-100, 100, 10);
        glVertex3d(100, 100, 10);
        glVertex3d(100, -100, 10);
    glEnd();*/

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
        oss << "Kubach. Try mouse buttons! FPS: " << (int)fps;
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

        sphere_hue = hue;
        sphere_brightness = brightness;
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
            {
                pl.y += 0.5;
                pl.vy = jump;
            }
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
            add_cube(0, 0, 0);
        keyEvent->accept();
    }
    else if (keyEvent->key() == Qt::Key_R)
    {
        pl.x = world_size * 0.5;
        pl.y = 5.0;
        pl.z = world_size * 0.5;
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
    else if (keyEvent->key() == Qt::Key_Q)
    {
        if (has_chosen_plane)
        {
            cubes[chosen_cube_index].planes[chosen_plane_index].brightness = discrete_brightness();
            cubes[chosen_cube_index].planes[chosen_plane_index].hue = discrete_hue();
        }
    }
    else if (keyEvent->key() == Qt::Key_E)
    {
        if (has_chosen_plane)
        {
            brightness = cubes[chosen_cube_index].planes[chosen_plane_index].brightness + 0.5 / sphere_y;
            hue = cubes[chosen_cube_index].planes[chosen_plane_index].hue - 3.0 / sphere_x;
        }
    }
}

void main_window::mousePressEvent (QMouseEvent * mouseEvent)
{
    if (mouseEvent->button() == Qt::MouseButton::RightButton)
    {
        if (has_chosen_plane)
        {
            cube_position to_add = cubes[chosen_cube_index].planes[chosen_plane_index].adjacent_cube();
            if (!pl.has_collision(to_add))
            {
                add_cube(to_add.x, to_add.y, to_add.z);
            }
        }
    }
    else if (mouseEvent->button() == Qt::MouseButton::LeftButton)
    {
        if (has_chosen_plane)
        {
            cube_positions.erase(cubes[chosen_cube_index]);
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

    bool old_on_surface = on_surface;
    double old_vy = pl.vy;

    on_surface = false;
    for (const cube & c : cubes)
        on_surface |= pl.collide(c);

    if (!old_on_surface && on_surface)
    {
        if (old_vy < -3.0)
            health = exp((old_vy + 3.0) * 0.4);
    }

    double sphere_k = 0.2;
    sphere_hue += sphere_k * (hue - sphere_hue);
    sphere_brightness += sphere_k * (brightness - sphere_brightness);

    health += 0.01;
    if (health > 1.0) health = 1.0;

    //updateGL();
    paintGL();
}
