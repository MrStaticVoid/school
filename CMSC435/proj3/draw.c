/* your drawing code goes in here */

/* by including draw.h, we ensure that the exported prototypes
   match the function definitions */
#include "draw.h"
#include "Draw.h"
#include "Globals.h"

// Apple's annoying non-standard GL include location
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <math.h>

// should we use per-vertex color or per-face?
static int smoothColor = 0;

/*
 * this is called every time the screen needs to be redrawn 
 */
void 
draw(void)
{
    /* clear old screen contents */
    glClearColor(0,0,0,1); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* draw something */
    glBegin(GL_TRIANGLES);
  
    /* For smooth colors, specify once per vertex, for
       faceted look, specify once per vertex
    */
    if (!smoothColor)  glColor3f(1,1,1);

    glNormal3f( 1, 1, 1);
    { /* braces not necessary, but help to visually group each triangle */
	if (smoothColor) glColor3f(1,1,1);
	glVertex3f(0,50,0); 

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(0,-50,100);

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(100,-50,0);
    }

    glNormal3f( 1, 1,-1);
    {
	if (smoothColor) glColor3f(1,1,1);
	glVertex3f(0,50,0); 

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(100,-50,0);

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(0,-50,-100); 
    }

    glNormal3f(-1, 1,-1);
    {
	if (smoothColor) glColor3f(1,1,1);
	glVertex3f(0,50,0); 

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(0,-50,-100); 

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(-100,-50,0); 
    }

    glNormal3f(-1, 1, 1);
    {
	if (smoothColor) glColor3f(1,1,1);
	glVertex3f(0,50,0); 

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(-100,-50,0);

	if (smoothColor) glColor3f(1,1,0);
	glVertex3f(0,-50,100); 
    }

    glEnd();

    /* or, for fun, uncomment this
       glPushMatrix();
       glTranslatef(-10,0,0);
       glutSolidTeapot(90);
       glPopMatrix();
    */
    
    /* tell OpenGL to finish drawing and switch the double buffer */
    glutSwapBuffers();
}

/* called on any keypress
 *
 * We don't use x and y, but they're the mouse location when the key
 * was pressed.
 */
void
key(unsigned char k, int x, int y)
{
    switch (k) {
    case 27:			/* Escape: exit */
	exit(0);
    }
}

void special(unsigned char k, int x, int y) {
    switch (k) {
    case GLUT_KEY_UP:
    	carx += 1.0 * sin(heading * PI / 180);
	carz -= 1.0 * cos(heading * PI / 180);
	glutPostRedisplay();
	break;
    case GLUT_KEY_DOWN:
    	carx -= 1.0 * sin(heading * PI / 180);
	carz += 1.0 * cos(heading * PI / 180);
	glutPostRedisplay();
	break;
    case GLUT_KEY_LEFT:
        heading--;
	if (heading > 360.0)
		heading -= 360.0;
	if (heading < 0.0)
		heading += 360.0;
	glutPostRedisplay();
	break;
    case GLUT_KEY_RIGHT:
    	heading++;
	if (heading > 360.0)
		heading -= 360.0;
	if (heading < 0.0)
		heading += 360.0;
	glutPostRedisplay();
	break;
    }
}
