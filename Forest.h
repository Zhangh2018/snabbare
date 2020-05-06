#pragma once

#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "loadobj.h"
#include "LoadTGA.h"


typedef struct {
    Model* model;
    GLuint texture;
    GLuint shader;
} Forest;

Forest* loadForest(float xmax, char* fileTexture, GLuint shader);
void drawForest(Forest* forest, mat4 worldToView);
