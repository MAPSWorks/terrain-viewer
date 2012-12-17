/**
 * mouse.c
 */
#include <stdio.h>
#include "terrain.h"
#include "mouse.h"

extern cameraData camera;

/**
 * Callback function to handle mouse movement events. Rotates camera on 
 * based on mouse position.
 */
void 
mouse_move(int x, int y) {
    if(camera.last_mouse_x != -1 && camera.last_mouse_y != -1) {
        int const diff_x = x - camera.last_mouse_x;
        int const diff_y = y - camera.last_mouse_y;

        camera.last_mouse_y = y;
        camera.last_mouse_x = x;

        // Diff in pixels, scale in degrees
        GLfloat const rot_scale = 0.05f;
        camera.theta[0] += (GLfloat)diff_y * rot_scale;
        camera.theta[1] += (GLfloat)diff_x * rot_scale;

        glutPostRedisplay();
    }
}

/**
 * Callback function to handle mouse click events. Changes state so that
 * any mouse movement rotates the camera when left button is down.
 */
void 
mouse_click(int button, int state, int x, int y) {
    if(button == GLUT_LEFT_BUTTON) {
        if(state == GLUT_DOWN) {
            camera.last_mouse_x = x;
            camera.last_mouse_y = y;
        }else {
            camera.last_mouse_x = -1;
            camera.last_mouse_y = -1;
        }
    }
}
