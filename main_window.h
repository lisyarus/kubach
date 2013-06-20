#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "player.h"
#include "cube.h"
#include "kubeman.h"

#include <QGLWidget>

#include <vector>
#include <chrono>
#include <queue>
#include <set>

class main_window : public QGLWidget
{
    Q_OBJECT

    int width, height;
    player pl;

    static constexpr double speed = 8;

    QPoint mouse_pos;

    double ratio;

    const double cross_size = 0.05;

    std::vector<cube> cubes;

    std::set<cube_position> cube_positions;

    static const int texture_size = 32;
    unsigned char texture[3 * texture_size * texture_size];
    unsigned int texture_id;

    bool has_chosen_plane;
    int chosen_cube_index;
    int chosen_plane_index;

    bool enable_gravity;
    const double g = 7;

    const double jump = 1.5;

    bool on_surface;

    static const int average_frames = 10;
    double last_frame;
    std::queue<std::chrono::high_resolution_clock::time_point> frames;

    std::vector<kubeman> kubemen;

    double brightness, hue;
    double discrete_brightness ( ) const;
    double discrete_hue ( ) const;

    color get_color (double brightness, double hue) const;
    color get_current_color ( ) const;
    void set_color (color c) const;

    void add_cube (int x, int y, int z);

    double sphere_hue, sphere_brightness;

    const int sphere_x = 12;
    const int sphere_y = 4;

    const int world_size = 50;

    bool rainbow;

    double health;
    
public:
    main_window(QGLWidget *parent = 0);
    ~main_window();

    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mouseMoveEvent(QMouseEvent * mouseEvent) override;
    void keyPressEvent(QKeyEvent * keyEvent) override;
    void keyReleaseEvent(QKeyEvent * keyEvent) override;

    void mousePressEvent (QMouseEvent * mouseEvent) override;
    void mouseReleaseEvent (QMouseEvent * mouseEvent) override;

    void wheelEvent (QWheelEvent * event) override;

    void timerEvent (QTimerEvent *);
};

#endif // MAIN_WINDOW_H
