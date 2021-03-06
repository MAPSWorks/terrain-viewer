/**
 * keyboard.c
 */
#include "terrain.h"
#include "keyboard.h"

// Global variables defined in init.c
extern worldData world;
extern cameraData camera;

/**
 * Callback function to handle key events
 *  ESC - exit
 *
 *  A/D - translate objects along X axis
 *  Q/E - translate objects along Y axis
 *  W/S - translate objects along Z axis
 *
 *  i/I - rotate along X axis
 *  o/O - rotate along Y axis
 *  p/P - rotate along Z axis
 *
 *  f - toggle wireframe
 *  g - cycle polygon fill
 *
 *  v/V - rotate sun along X axis
 * 
 *  1/! - increase/decrease shininess
 */
void keyboard( unsigned char key, int x, int y ) {
    GLfloat const DegreesToRadians = M_PI / 180.0;
    GLfloat step = world.cube_size * 0.01; // Amount to translate per step
    GLfloat angleStep = 5.0; // Amount to rotate per step
    GLfloat const sunAngleStep = 5.0; // Amount sun moves in degrees per step

    switch(key) {
        case 033:
            exit( EXIT_SUCCESS );
            break;
        case 'W': // Move forward in direction of camera face
            step /= 5.0;
        case 'w':
            camera.viewer[0] -= step * -sin(camera.theta[1] * DegreesToRadians) 
                                    * cos(camera.theta[0] * DegreesToRadians); 
            camera.viewer[1] -= step * sin(camera.theta[0] * DegreesToRadians); 
            camera.viewer[2] -= step * cos(camera.theta[1] * DegreesToRadians) 
                                    * cos(camera.theta[0] * DegreesToRadians);
            break;
        case 'S': // Move backward from direction of camera face
            step /= 5.0;
        case 's':
            camera.viewer[0] += step * -sin(camera.theta[1] * DegreesToRadians) 
                                    * cos(camera.theta[0] * DegreesToRadians);
            camera.viewer[1] += step * sin(camera.theta[0] * DegreesToRadians); 
            camera.viewer[2] += step * cos(camera.theta[1] * DegreesToRadians) 
                                    * cos(camera.theta[0] * DegreesToRadians);
            break;
        case 'D': // Rotate camera clockwise around y axis
            angleStep /= 5.0;
        case 'd': 
            camera.theta[1] += angleStep;
            break;
        case 'A': // Rotate camera counter-clockwise around y axis
            angleStep /= 5.0;
        case 'a': 
            camera.theta[1] -= angleStep;
            break;
        case 'E': // Rotate camera up
            angleStep /= 5.0;
        case 'e':
            camera.theta[0] -= angleStep;
            break;
        case 'Q': // Rotate camera down
            angleStep /= 5.0;
        case 'q': 
            camera.theta[0] += angleStep;
            break;
        // Axis Rotations (+/-)
        case 'i':  // x axis
            camera.theta[0] += angleStep;
            break;
        case 'I':
            camera.theta[0] -= angleStep;
            break;
        case 'o':  // y axis
            camera.theta[1] += angleStep;
            break;
        case 'O':
            camera.theta[1] -= angleStep;
            break;
        case 'p': // z axis
            camera.theta[2] += angleStep;
            break;
        case 'P':
            camera.theta[2] -= angleStep;
            break;
        case 'f': // Toggle between wireframe modes
            if(world.wireframe_mode == 0) {
                world.wireframe_mode = 1;
            }else {
                world.wireframe_mode = 0;
            }
            break;
        case 'g': // Cycle through polygon fill modes
            if(++world.fill_mode > 2) {
                world.fill_mode = 0;
            }
            break;
        case 'v': // Rotate sun
            world.sun_theta += sunAngleStep;
            break;
        case 'V': 
            world.sun_theta -= sunAngleStep;
            break;
        case '1': // Change terrain light properties
            world.ground_material.shininess += 10.0;
            break;
        case '!':
            world.ground_material.shininess -= 10.0;
            if(world.ground_material.shininess < 1.0) {
                world.ground_material.shininess = 1.0;
            }
            break;
        default:
            return; // Don't redisplay if nothing updated
            break;
    }

    // Ask nicely for a redraw
    glutPostRedisplay();
}
