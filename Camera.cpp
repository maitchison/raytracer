#include "Camera.h"

Camera::Camera(glm::vec3 location) : SceneObject(location)
{
}


void Camera::calculateLighting(RayIntersectionResult intersection, Light* light, Color& ambientLightSum, Color& diffuseLightSum, Color& specularLightSum)
{
    Material* material = intersection.target->material;

    // diffuse light    
    glm::vec3 lightVector = glm::normalize(light->getLocation() - intersection.location);    
    float diffusePower = glm::dot(lightVector, intersection.normal);
    if (diffusePower < 0) diffusePower = 0;
    
    // specular light
    glm::vec3 reflVector = glm::reflect(-lightVector, intersection.normal);
    glm::vec3 viewVector = glm::normalize(this->location - intersection.location);

    float specularPower = glm::dot(reflVector, viewVector);
    if (specularPower < 0) {
        specularPower = 0;
    } else {
        specularPower = (float)pow(specularPower, material->shininess);
    }

    Color diffuseLight = light->color * diffusePower;
    Color specularLight = light->color * specularPower;
    
    // this is an optimization.  Points on the far side of a sphere, for example, will have no lighting
    // so there is no need to do shadow calculations.  
    bool needsShadow = light->shadow && (diffusePower > EPSILON || specularPower > EPSILON);
            
    // shadow
    // note: i'm 90% sure I should be doing the transpariency checks in the other order, i.e. absorb from objects closest 
    // to the light first.  Hovever this will often not be noticiable (I think... ?)
    if (needsShadow) 
    {
        float lightDistance;
        glm::vec3 shadowTestPoint = intersection.location;

        // handle transparient shadows by letting ray continue when meeting a transparient object
        for (int i = 0; i < 9; i++) {            
            // offsetting the shadow trace a little stops self shadowing artifacts
            Ray shadow(shadowTestPoint + lightVector * 0.01f, lightVector);
            shadow.shadowTrace=true; // this will ignore objects that do not cast shadows.

            RayIntersectionResult shadowIntersection = scene->intersect(shadow);    
            lightDistance = glm::length(light->getLocation() - shadowTestPoint);

            if (shadowIntersection.didCollide() && shadowIntersection.t < lightDistance) {
                // we sample the uv, so that textured transpariency will work :)
                glm::vec2 uv = glm::vec2(0,0);
                if (material->needsUV()) {        
                    uv = intersection.target->getUV(intersection.location);        
                }
                Color occluderColor = shadowIntersection.target->material->getDiffuseColor(uv);
                float transmission = 1.0f - occluderColor.a;
                diffuseLight *= (transmission * occluderColor);
                specularLight *= (transmission * occluderColor);
                // no need to continue if we hit a solid object.
                if (transmission < EPSILON) break;
                shadowTestPoint = shadowIntersection.location;                
            } else {
                // didn't hit anything so stop.
                break;
            }


        }
    }

    // accumulate light.
    ambientLightSum += light->ambientLight * light->color;
    diffuseLightSum += diffuseLight * light->color;
    specularLightSum += specularLight * light->color;
}

Color Camera::trace(Ray ray, int depth)
{    
    RayIntersectionResult intersection = scene->intersect(ray);

    if (!intersection.didCollide()) return backgroundColor;      //If there is no intersection return background colour

    Material* material = intersection.target->material;

    // we check if uv co-ords need calculating as they can be quite slow (transidental functions for example).  
    glm::vec2 uv = glm::vec2(0,0);
    if (material->needsUV()) {        
        uv = intersection.target->getUV(intersection.location);        
    }
    
    // modify normal vector based on normal map (if required)
    if (material->normalTexture) {
        // we find the 3 vectors required to transform the normal map from object space to world space.
        glm::vec3 normalVector = intersection.normal;
        glm::vec3 materialNormal = glm::vec3(material->normalTexture->sampleNormalMap(uv) * 2.0f - 1.0f);

        // soften the normal map a little
        materialNormal = glm::normalize(materialNormal + 1.0f * glm::vec3(0,0,1));

        glm::vec3 normal = normalVector;
        glm::vec3 tangent = intersection.target->getTangent(intersection.location);
        glm::vec3 bitangent = glm::cross(normal, tangent);
        normalVector = glm::vec3(
            materialNormal.x*tangent + materialNormal.y*bitangent + materialNormal.z*normal             
        );

        // update the intersection normal vector
        intersection.normal = normalVector;
    }

    // sum up the lighting from all lights.
    Color ambientLight = Color(0,0,0,1);
    Color diffuseLight = Color(0,0,0,1);
    Color specularLight = Color(0,0,0,1);
    for (int i = 0; i < scene->lights.size(); i++) {        
        calculateLighting(intersection, scene->lights[i], ambientLight, diffuseLight, specularLight);        
    }

    // combine lighting
    Color materialColor = material->getDiffuseColor(uv);
    Color color = (ambientLight + diffuseLight) * materialColor + specularLight + material->emisiveColor;
    
    // reflection    
    if(material->reflectivity > 0 && depth < MAX_STEPS) {
        glm::vec3 reflectedDir = glm::reflect(ray.dir, intersection.normal);
        
        // add bluring
        if (material->reflectionBlur > EPSILON) {            
            reflectedDir = defocus(reflectedDir, material->reflectionBlur);
        }

        Ray reflectedRay(intersection.location + reflectedDir * EPSILON, reflectedDir);
        Color reflectedCol = trace(reflectedRay, depth+1); 
        color += (material->reflectivity*reflectedCol);        
    }

    if (materialColor.a < 1) {        
        if (material->refractionIndex == 1.0) {        

            // ----------------
            // transparency 
    
            // start the ray a little further on from where we hit.
            Ray transmittedRay = Ray(intersection.location + 0.001f * ray.dir, ray.dir);
            Color transmittedCol = trace(transmittedRay, depth); 
            color += (1.0f-materialColor.a)*transmittedCol;
            
        } else {            

            // ----------------
            // refraction
    
            glm::vec3 refractedDir = glm::refract(ray.dir, intersection.normal, 1.0f/material->refractionIndex);

            Ray refractedRay = Ray(intersection.location + refractedDir * 0.001f , refractedDir);
            
            // the refracted ray will exit the object at this location, don't trace against entire scene, just trace against the 
            // specific object (faster, and less prone to error).
            RayIntersectionResult exitPoint = intersection.target->intersect(refractedRay);

            if (exitPoint.didCollide()) {
                glm::vec3 exitDir = glm::refract(refractedDir, -exitPoint.normal, material->refractionIndex);
                Ray exitRay = Ray(exitPoint.location + exitDir * 0.001f, exitDir);
                Color refractedCol = trace(exitRay, depth+1); 
                color += (1.0f-materialColor.a)*refractedCol;
            } else {
                // this case shouldn't happen, but might due to rounding... just ignore (i.e. use black color)
                color = Color(1,0,1,1);
            }
        }
    }
    
	return color;
}

/** Renders given number of pixels before returning control. */
int Camera::render(int pixels, int oversample, float defocusBlur, bool autoReset)
{	
	int totalPixels = SCREEN_WIDTH * SCREEN_HEIGHT;
	float aspectRatio = float(SCREEN_WIDTH / SCREEN_HEIGHT);

	if (pixels == -1) {
		pixels = totalPixels - pixelOn;
	}
	
	int pixelsDone = 0;

    // camera rotation matrix
    glm::mat4x4 rotationMatrix = EulerRotationMatrix(rotation);
    
	#pragma loop(hint_parallel(4))  
	for (int i = 0; i < pixels; i++) {

		pixelOn++;		

		if (pixelOn >= totalPixels)
		{
			// reset.
			if (autoReset) pixelOn = 0;
			return i;
		}

		int x = 0;
		int y = 0;

		x = pixelOn % SCREEN_WIDTH;
		y = pixelOn / SCREEN_WIDTH;
		
		Color outputCol = Color(0, 0, 0, 1);

		for (int j = 0; j < oversample; j++) {
			float jitterx = (oversample == 1) ? 0.5 : randf();
			float jittery = (oversample == 1) ? 0.5 : randf();

			// find the rays direction
			float rx = (2 * ((x + jitterx) / SCREEN_WIDTH) - 1) * tan(fov / 2 * M_PI / 180) * aspectRatio;
			float ry = (1 - 2 * ((y + jittery) / SCREEN_HEIGHT)) * tan(fov / 2 * M_PI / 180);
			glm::vec3 dir = glm::normalize(glm::vec3(rx, -ry, -1));

            // apply camera tranform
            dir = glm::vec3(glm::vec4(dir.x, dir.y, dir.z, 0.0) * rotationMatrix);

            // defocus
            if (defocusBlur > EPSILON) {
                dir = defocus(dir, defocusBlur);
            }
			
			Ray ray = Ray(location, dir);
			Color col = trace(ray);
			outputCol = outputCol + (col * (1.0f/oversample));
		}
				
        gfx.putPixel(x, y, outputCol);
        gfx.putPixel(x, y+2, Color(1,1,1,1));
        gfx.putPixel(x, y+1, Color(0,0,0,1));
        gfx.putPixel(x, y+3, Color(0,0,0,1));
    
		pixelsDone++;

	}
	return pixelsDone; 
}