/*========================================================================
* COSC 363  Computer Graphics (2018)
* Ray tracer 
* See Lab07.pdf for details.
*=========================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "Plane.h"
#include "SceneObject.h"
#include "Ray.h"
#include "math.h"
#include <GL/glut.h>
using namespace std;

const float WIDTH = 20.0;  
const float HEIGHT = 20.0;
const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -WIDTH * 0.5;
const float XMAX =  WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX =  HEIGHT * 0.5;

vector<SceneObject*> sceneObjects;  //A global list containing pointers to objects in the scene


//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);
	glm::vec3 light(10, 40, -3);
	glm::vec3 ambientCol(0.2);   //Ambient color of light

    float shinyness = 4.0;

    ray.closestPt(sceneObjects);		//Compute the closest point of intersetion of objects with the ray

    if(ray.xindex == -1) return backgroundCol;      //If there is no intersection return background colour

    glm::vec3 materialCol = sceneObjects[ray.xindex]->getColor(); 

    // diffuse light
    glm::vec3 normalVector = sceneObjects[ray.xindex]->normal(ray.xpt);
    glm::vec3 lightVector = glm::normalize(light - ray.xpt);
    float diffuseLight = glm::dot(lightVector, normalVector);
    if (diffuseLight < 0) diffuseLight = 0;

    // specular light
    glm::vec3 reflVector = glm::reflect(-lightVector, normalVector);
    glm::vec3 viewVector = glm::normalize(glm::vec3(ray.xpt.x, ray.xpt.y, -ray.xpt.z));
    float specularLight = glm::dot(reflVector, viewVector);
    if (specularLight < 0) {
        specularLight = 0;
    } else {
        specularLight = (float)pow(specularLight, shinyness);
    }

    // shadow
    Ray shadow(ray.xpt, lightVector);
    shadow.closestPt(sceneObjects);
    if (shadow.xindex >= 0 && shadow.xdist < lightVector.length()) {
        diffuseLight = specularLight = 0;
    }

    // apply the lighting model
    glm::vec3 colorSum = ambientCol * materialCol + diffuseLight * materialCol + specularLight;
    
    // reflection
    if(ray.xindex == 0 && step < MAX_STEPS) {
        glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVector);
        Ray reflectedRay(ray.xpt, reflectedDir);
        glm::vec3 reflectedCol = trace(reflectedRay, step+1); //Recursion!
        colorSum = colorSum + (0.8f*reflectedCol);
    }

	return colorSum;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX-XMIN)/NUMDIV;  //cell width
	float cellY = (YMAX-YMIN)/NUMDIV;  //cell height

	glm::vec3 eye(0., 0., 0.);  //The eye position (source of primary rays) is the origin

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a quad.

	for(int i = 0; i < NUMDIV; i++)  	//For each grid point xp, yp
	{
		xp = XMIN + i*cellX;
		for(int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j*cellY;

		    glm::vec3 dir(xp+0.5*cellX, yp+0.5*cellY, -EDIST);	//direction of the primary ray

		    Ray ray = Ray(eye, dir);		//Create a ray originating from the camera in the direction 'dir'
			ray.normalize();				//Normalize the direction of the ray to a unit vector
		    glm::vec3 col = trace (ray, 1); //Trace the primary ray and get the colour value

			glColor3f(col.r, col.g, col.b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp+cellX, yp);
			glVertex2f(xp+cellX, yp+cellY);
			glVertex2f(xp, yp+cellY);
        }
    }

    glEnd();
    glFlush();
}


//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
    glClearColor(0, 0, 0, 1);

	//-- Create a pointer to a sphere object
	Sphere *sphere1 = new Sphere(glm::vec3(-5.0, -5.0, -90.0), 15.0, glm::vec3(0, 0, 1));
    Sphere *sphere2 = new Sphere(glm::vec3(+4.0, +3.0, -50.0), 4.0, glm::vec3(1, 0, 0));
    Sphere *sphere3 = new Sphere(glm::vec3(+8.0, -8.0, -70.0), 4.0, glm::vec3(0, 1, 0));

    Plane *plane = new Plane (glm::vec3(-20., -20, -40), //Point A
        glm::vec3(20., -20, -40), //Point B
        glm::vec3(20., -20, -200), //Point C
        glm::vec3(-20., -20, -200), //Point D
        glm::vec3(0.5, 0.5, 0) //Colour
    ); 

	//--Add the above to the list of scene objects.
	sceneObjects.push_back(sphere1); 
    sceneObjects.push_back(sphere2); 
    sceneObjects.push_back(sphere3); 
    sceneObjects.push_back(plane); 
}



int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(600, 600);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracer");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
