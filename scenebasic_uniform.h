#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/plane.h"
#include "helper/objmesh.h"

#include "helper/skybox.h";

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog;

    GLuint fsQuad;
    GLuint hdrFbo, blurFbo;
    GLuint hdrTex, tex1, tex2;
    GLuint baseTex, normalMap;
    GLuint linearSampler, nearestSampler;

    std::unique_ptr<ObjMesh> mesh;
    float tPrev, angle, rotSpeed;
    int bloomBufWidth, bloomBufHeight;

    void compile();
    void setMatrices();
    void drawScene();

    void setupFBO();
    void pass1();
    void pass2();
    void pass3();
    void pass4();
    void pass5();

    float gauss(float, float);
    void computeLogAveLuminance();

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
