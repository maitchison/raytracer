/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The sphere class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#include "Sphere.h"
#include <math.h>
#include <stdio.h>



/**
* Sphere's intersection method.  The input is a ray (pos, dir). 
*/
RayIntersectionResult Sphere::intersect(Ray ray)
{    
    glm::vec3 vdif = ray.pos - center;
    float b = glm::dot(ray.dir, vdif);
    float len2 = glm::dot(vdif, vdif);
    float c = len2 - radius*radius;
    float delta = b*b - c;
   
	if (delta < 0.0001f) return RayIntersectionResult();

    float t1 = -b - sqrt(delta);
    float t2 = -b + sqrt(delta);
    
    float t = -1;
    
    if ((t1 >= 0) && ((t1 <= t2) || (t2 < 0))) t = t1;
    if ((t2 >= 0) && ((t2 <= t1) || (t1 < 0))) t = t2;    

    // ray does not intersect
    if (t < 0) {        
        return RayIntersectionResult();
    } 

    RayIntersectionResult result = RayIntersectionResult();
    result.target = this;
    result.t = t;
    result.location = ray.pos + ray.dir * t;
    result.normal = normal(result.location);
    
    return result;
}

/**
* Returns the unit normal vector at a given point.
* Assumption: The input point p lies on the sphere.
*/
glm::vec3 Sphere::normal(glm::vec3 p)
{
    return glm::normalize(p - center);    
}
