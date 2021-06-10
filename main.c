#ifdef MACOS
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "glm.h"

#define MAX_BODIES 25
#define TOP_VIEW 1
#define ECLIPTIC_VIEW 2
#define EXPLORE_MOOD 3
#define EARTH_VIEW 4
#define PI 3.141593
#define DEG_TO_RAD 0.01745329
#define ORBIT_POLY_SIDES 50
#define DISTANCE 200000000
#define SUN 0
#define TURN_ANGLE 4.0

typedef struct {
    char    name[MAX_BODIES]; /* name */
    GLfloat r,g,b;          /* colour */
    GLfloat orbital_radius; /* distance to parent body (km) */
    GLfloat orbital_tilt;   /* angle of orbit wrt ecliptic (deg) */
    GLfloat orbital_period; /* time taken to orbit (days) */
    GLfloat radius;         /* radius of body (km) */
    GLfloat axis_tilt;      /* tilt of axis wrt body's orbital plane (deg) */
    GLfloat rot_period;     /* body's period of rotation (days) */
    GLint   orbits_body;    /* identifier of parent body */

    GLfloat spin;
    /* current spin value (deg) */
    GLfloat orbit;          /* current orbit value (deg) */
} body;

body  bodies[MAX_BODIES];
int   numBodies, current_view, draw_axes, draw_orbits, draw_labels, draw_starfield;
double  date, eyex, eyey, eyez, centrex, centrey, centrez, upx, upy, upz, shipx, shipy, shipz;
GLdouble lon, shipLon;
GLdouble lat, shipLat;
GLMmodel* enterprise;
float time_step = 0.5;
float move_speed = 0.01;
int size_shape = 20;

float myRand(void) {
    /* return a random float in the range [0,1] */
    return (float) rand() / RAND_MAX;
}

GLfloat randPosition() {
    return (GLfloat) (myRand() - 0.5) * 2 * 300000000;
}

void drawStarfield(void) {
    int i = 0;
    static bool positionGenerated = false;
    static GLfloat positions[3000];

    // We only generate these value once.
    if (!positionGenerated) {
        positionGenerated = true;
        for (; i < 3000; i++) {
            positions[i] = randPosition();
        }
    }

    i = 0;
    for (; i < 1000; i++) {
        glBegin(GL_POINTS);
        glColor3f(1.0, 1.0, 1.0);
        glVertex3f(positions[3 * i], positions[3 * i + 1], positions[3 * i + 2]);
        glEnd();
    }
}

void readSystem(void) {
    /* reads in the description of the solar system */

    FILE *f;
    int i;

    f= fopen("Resources/sys", "r");
    if (f == NULL) {
        printf("ex2.c: Couldn't open 'sys'\n");
        printf("To get this file, use the following command:\n");
        printf("  cp /opt/info/courses/COMP27112/ex2/sys .\n");
        exit(0);
    }
    fscanf(f, "%d", &numBodies);
    for (i= 0; i < numBodies; i++) {
        fscanf(f, "%s %f %f %f %f %f %f %f %f %f %d",
               bodies[i].name,
               &bodies[i].r, &bodies[i].g, &bodies[i].b,
               &bodies[i].orbital_radius,
               &bodies[i].orbital_tilt,
               &bodies[i].orbital_period,
               &bodies[i].radius,
               &bodies[i].axis_tilt,
               &bodies[i].rot_period,
               &bodies[i].orbits_body);

        /* Initialise the body's state */
        bodies[i].spin= 0.0;
        bodies[i].orbit= myRand() * 360.0; /* Start each body's orbit at a
                                          random angle */
        bodies[i].radius*= 1000.0; /* Magnify the radii to make them visible */
    }
    fclose(f);
    enterprise = glmReadOBJ("Resources/NCC1701/NCC1701Origin.obj");
}

void drawString(void* font, float x, float z, char* string) {
    char* ch = string;
    glRasterPos3f(x, 0.0, z);
    for (; *ch; ch++)
        glutBitmapCharacter(font, (int) *ch);
}

void calculate_lookat_point() {
    centrex = eyex + cos(lat * DEG_TO_RAD) * sin(lon * DEG_TO_RAD);
    centrey = eyey + sin(lat * DEG_TO_RAD);
    centrez = eyez + cos(lat * DEG_TO_RAD) * cos(lon * DEG_TO_RAD);
}

void drawShip() {
    glTranslated(shipx, shipy, shipz);
    glRotatef(shipLon, 0.0, 1.0, 0.0);
    glRotatef(shipLat, 1.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
    glmUnitize(enterprise);
    glmScale(enterprise, 0.01 * DISTANCE);
    glmDraw(enterprise, GLM_NONE);
}

void setView(void) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    switch (current_view) {
        case TOP_VIEW:
            eyex = 0.0;
            eyey = 2 * DISTANCE;
            eyez = 0.0;
            centrex = 0.0;
            centrey = 0.0;
            centrez = 0.0;
            upx = 0.0;
            upy = 0.0;
            upz = -1.0;
            break;
        case ECLIPTIC_VIEW:
            eyex = 2 * DISTANCE;
            eyey = 0.0;
            eyez = 0.0;
            centrex = 0.0;
            centrey = 0.0;
            centrez = 0.0;
            upx = 0.0;
            upy = 1.0;
            upz = 0.0;
            break;
        case EXPLORE_MOOD:
            calculate_lookat_point();
            break;
        case EARTH_VIEW:
            /*
             * Convert from Spherical Coordinate System to Cartesian Coordinate System.
             * x = rsinθcosφ.
             * y = rsinθsinφ.
             * z = rcosθ.
             */
            eyex = (bodies[3].orbital_radius + bodies[3].radius + 40000000)
                   * sin(DEG_TO_RAD * (90 + bodies[3].orbit));
            eyey = bodies[3].radius + 2000000;
            eyez = (bodies[3].orbital_radius + bodies[3].radius + 40000000)
                   * cos(DEG_TO_RAD * (90 + bodies[3].orbit));
            centrex = 0.0;
            centrey = 0.0;
            centrez = 0.0;
            upx = 0.0;
            upy = 1.0;
            upz = 0.0;
            break;
        default:
            break;
    }
    gluLookAt(eyex, eyey, eyez,
              centrex, centrey, centrez,
              upx, upy, upz);
}

void menu(int menuentry) {
    switch (menuentry) {
        case 1:
            current_view = TOP_VIEW;
            break;
        case 2:
            current_view = ECLIPTIC_VIEW;
            break;
        case 3:
            current_view = EXPLORE_MOOD;
            eyex = DISTANCE;
            eyey = 0.25 * DISTANCE;
            eyez = 0.25 * DISTANCE;
            shipx = 192763596.977131;
            shipy = 46818241.568945;
            shipz = 48563347.015047;
            centrex = 0.0;
            centrey = 0.0;
            centrez = 0.0;
            upx = 0.0;
            upy = 1.0;
            upz = 0.0;
            lat = -16.0;
            lon = -102.0;
            shipLat = 20.0;
            shipLon = -20.0;
            break;
        case 4:
            current_view = EARTH_VIEW;
            break;
        case 5:
            draw_labels = !draw_labels;
            break;
        case 6:
            draw_orbits = !draw_orbits;
            break;
        case 7:
            draw_starfield = !draw_starfield;
            break;
        case 8:
            exit(0);
        default:
            break;
    }
}

void init(void) {
    /* Define background colour */
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glutCreateMenu (menu);
    glutAddMenuEntry ("Top view", 1);
    glutAddMenuEntry ("Ecliptic view", 2);
    glutAddMenuEntry ("Explore mood", 3);
    glutAddMenuEntry ("Earth view", 4);
    glutAddMenuEntry ("", 999);
    glutAddMenuEntry ("Toggle labels", 5);
    glutAddMenuEntry ("Toggle orbits", 6);
    glutAddMenuEntry ("Toggle starfield", 7);
    glutAddMenuEntry ("", 999);
    glutAddMenuEntry ("Quit", 8);
    glutAttachMenu (GLUT_RIGHT_BUTTON);

    current_view= TOP_VIEW;
    draw_labels= 1;
    draw_orbits= 1;
    draw_starfield= 1;
    lat = 0.0;
    lon = 0.0;
}

void animate(void) {
    int i;

    date += time_step;

    for (i= 0; i < numBodies; i++)  {
        bodies[i].spin += 360.0 * time_step / bodies[i].rot_period;
        bodies[i].orbit += 360.0 * time_step / bodies[i].orbital_period;
        glutPostRedisplay();
    }
}

void drawOrbit(int n) {
    glColor3f(bodies[n].r, bodies[n].g, bodies[n].b);
    glRotatef(bodies[n].orbital_tilt, 1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    int i = 0;
    float x, z;
    for (; i < ORBIT_POLY_SIDES; i++) {
        x = bodies[n].orbital_radius * cos(i * ((2 * PI) / ORBIT_POLY_SIDES));
        z = bodies[n].orbital_radius * sin(i * ((2 * PI) / ORBIT_POLY_SIDES));
        glVertex3f(x, 0.0, z);
    }
    glEnd();
}

void drawLabel(int n) {
    glColor3f(bodies[n].r, bodies[n].g, bodies[n].b);
    float x = bodies[n].orbital_radius * sin((90 + bodies[n].orbit) * DEG_TO_RAD);
    float z = bodies[n].orbital_radius * cos((90 + bodies[n].orbit) * DEG_TO_RAD);
    drawString(GLUT_BITMAP_HELVETICA_18, x, z, bodies[n].name);
}

body getMother(body b) {
    return bodies[b.orbits_body];
}

void drawBody(int n) {
    glColor3f(bodies[n].r, bodies[n].g, bodies[n].b);
    if (bodies[n].orbits_body != SUN) {
        glRotatef(getMother(bodies[n]).orbital_tilt, 1.0, 0.0, 0.0);
        glRotatef(getMother(bodies[n]).orbit, 0.0, 1.0, 0.0);
        glTranslatef(getMother(bodies[n]).orbital_radius, 0.0, 0.0);
    }

    if (draw_orbits)
        drawOrbit(n);

    if (draw_labels)
        drawLabel(n);

    // Spin around the orbit.
    glRotatef(bodies[n].orbit, 0.0, 1.0, 0.0);

    // Translation along the orbit.
    glTranslatef(bodies[n].orbital_radius, 0.0, 0.0);

    // Orbital tilt.
    glRotatef(bodies[n].orbital_tilt, 1.0, 0.0, 0.0);

    // This line is NOT an error! We use translate + rotate to simulate orbiting,
    // but when doing glRotate(orbit), the body is also self rotated by orbit angle
    // unfortunately and unwillingly. We first let the body do a self rotate by
    // -orbit angle to cancel out this side effect.
    //
    // If you don't believe me, try comment out this line, and then moon will rotate
    // more than once in a month!
    glRotatef(-bodies[n].orbit, 0.0, 1.0, 0.0);

    // Axis tilt.
    glRotatef(bodies[n].axis_tilt, 1.0, 0.0, 0.0);

    // Spin around the body's axis.
    glRotatef(bodies[n].spin, 0.0, 1.0, 0.0);

    // Rotate by 90 degree.
    glRotatef(90, 1.0, 0.0, 0.0);

    glutWireSphere(bodies[n].radius, size_shape, size_shape / 2);
    glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 3 * bodies[n].radius);
    glVertex3f(0.0, 0.0, -3 * bodies[n].radius);
    glEnd();
}

void drawAxis() {
    glBegin(GL_LINES);
    // Red.
    glColor3f(1.0, 0.0, 0.0);
    // X.
    glVertex3d(2 * DISTANCE, 0.0, 0.0);
    glVertex3d(0.0, 0.0, 0.0);
    glEnd();

    glBegin(GL_LINES);
    // Green.
    glColor3f(0.0, 1.0, 0.0);
    // Y.
    glVertex3d(0.0, 2 * DISTANCE, 0.0);
    glVertex3d(0.0, 0.0, 0.0);
    glEnd();

    glBegin(GL_LINES);
    // Blue.
    glColor3f(0.078, 0.835, 0.96);
    // Z.
    glVertex3d(0.0, 0.0, 2 * DISTANCE);
    glVertex3d(0.0, 0.0, 0.0);
    glEnd();
}

void display(void) {
    int i;

    glClear(GL_COLOR_BUFFER_BIT);

    /* set the camera */
    setView();

    if (draw_starfield)
        drawStarfield();

    if (draw_axes)
        drawAxis();

    for (i= 0; i < numBodies; i++) {
        glPushMatrix();
        drawBody(i);
        glPopMatrix();
    }

    if (current_view == EXPLORE_MOOD)
        drawShip();

    glutSwapBuffers();

}

void reshape(int w, int h) {
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective (48.0, (GLfloat)w/(GLfloat)h,  10000.0, 800000000.0);
}

void keyboard(unsigned char key, int x, int y) {
    switch(key)
    {
        case 27:  /* Escape key */
            glmDelete(enterprise);
            exit(0);
        case 'a':
            draw_axes = !draw_axes;
            break;
        case '-':
            if (time_step - 0.01 > 0.01)
                time_step -= 0.01;
            break;
        case '+':
            if (time_step + 0.01 < 10)
                time_step += 0.01;
            break;
        case 'u':
            if (current_view == EXPLORE_MOOD) {
                eyey += move_speed * DISTANCE;
                centrey += move_speed * DISTANCE;
            }
            break;
        case 'd':
            if (current_view == EXPLORE_MOOD) {
                eyey -= move_speed * DISTANCE;
                centrey -= move_speed * DISTANCE;
            }
            break;
        case 'f':
            if (current_view == EXPLORE_MOOD) {
                eyex += move_speed * DISTANCE * sin(DEG_TO_RAD * lon);
                eyey += move_speed * DISTANCE * sin(DEG_TO_RAD * lat);
                eyez += move_speed * DISTANCE * cos(DEG_TO_RAD * lon);
            }
            break;
        case 'b':
            if (current_view == EXPLORE_MOOD) {
                eyex -= move_speed * DISTANCE * sin(DEG_TO_RAD * lon);
                eyey -= move_speed * DISTANCE * sin(DEG_TO_RAD * lat);
                eyez -= move_speed * DISTANCE * cos(DEG_TO_RAD * lon);
            }
            break;
        case 'l':
            if (current_view == EXPLORE_MOOD) {
                eyex += move_speed * DISTANCE * sin(DEG_TO_RAD * (lon + 90));
                eyez += move_speed * DISTANCE * cos(DEG_TO_RAD * (lon + 90));
            }
            break;
        case 'r':
            if (current_view == EXPLORE_MOOD) {
                eyex += move_speed * DISTANCE * sin(DEG_TO_RAD * (lon - 90));
                eyez += move_speed * DISTANCE * cos(DEG_TO_RAD * (lon - 90));
            }
            break;
        case ' ':
            if (current_view == EXPLORE_MOOD) {
                shipx += move_speed * DISTANCE * sin(DEG_TO_RAD * (shipLon + 180));
                shipy += move_speed * DISTANCE * sin(DEG_TO_RAD * (shipLat));
                shipz += move_speed * DISTANCE * cos(DEG_TO_RAD * (shipLon + 180));
            }
            break;
        case '8':
            if (current_view == EXPLORE_MOOD)
                shipLat += TURN_ANGLE;
            break;
        case '2':
            if (current_view == EXPLORE_MOOD)
                shipLat -= TURN_ANGLE;
            break;
        case '4':
            if (current_view == EXPLORE_MOOD)
                shipLon += TURN_ANGLE;
            break;
        case '6':
            if (current_view == EXPLORE_MOOD)
                shipLon -= TURN_ANGLE;
            break;
        default:
            printf("NOT IMPLEMENTED! KEY: %d\n", key);
    }
}

void cursor_keys(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_PAGE_DOWN:
            if (size_shape - 1 > 3)
                size_shape--;
            break;
        case GLUT_KEY_PAGE_UP:
            if (size_shape + 1 < 50)
                size_shape++;
            break;
        case GLUT_KEY_LEFT:
            if (current_view == EXPLORE_MOOD)
                lon += TURN_ANGLE;
            break;
        case GLUT_KEY_RIGHT:
            if (current_view == EXPLORE_MOOD)
                lon -= TURN_ANGLE;
            break;
        case GLUT_KEY_UP:
            if (current_view == EXPLORE_MOOD)
                if (lat + TURN_ANGLE < 90)
                    lat += TURN_ANGLE;
            break;
        case GLUT_KEY_DOWN:
            if (current_view == EXPLORE_MOOD)
                if (lat - TURN_ANGLE > -90)
                    lat -= TURN_ANGLE;
            break;
        default:
            printf("NOT IMPLEMENTED! KEY: %d\n", key);
    }
}

void mouse_whell(int button, int direction, int x, int y) {
    // printf("Test! %d %d\n", button, direction);
    if (button == 3) {
        // The up direction of mouse wheel.
        if (move_speed + 0.005 < 0.5)
            move_speed += 0.005;
    }

    if (button == 4) {
        // The down direction of mouse wheel.
        if (move_speed - 0.005 > 0.005)
            move_speed -= 0.005;
    }

}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("COMP27112 Exercise 2");
    glutFullScreen();
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(cursor_keys);
    glutMouseFunc(mouse_whell);
    glutIdleFunc(animate);
    readSystem();
    glutMainLoop();
    return 0;
}
