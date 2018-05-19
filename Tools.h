#pragma once

/** Some extentions to the GLM library */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <fstream>

namespace glm {

    /** Returns squared length of vector */
    float length2(glm::vec3 v);   
}

/** Loads a file from disk into at std::vector */
void loadFile(std::vector<unsigned char>& buffer, const std::string& filename);

/** Returns x cliped to between a and b. */
float clip(float x, float a, float b);
