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
#include "irrklang.h"

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog;
    GLSLProgram noiseProg;
    GLSLProgram skyboxProg;

    GLuint fsQuad;
    GLuint renderFBO, intermediateFBO;
    GLuint renderTex, intermediateTex;

    GLuint noiseQuad;
    GLuint noiseTex;

    SkyBox skybox;

    std::unique_ptr<ObjMesh> mesh;
    float tPrev, angle, rotSpeed;
    bool blurEnabled; 
	float timeSincePress;

    // Audio
    irrklang::ISoundEngine* soundEngine;
    float volume;

    void compile();
    void setMatrices(int type); // 1 = Scene, 0 = Noise
    void setTextures();
    void setupQuadBuffers();
    void setupUniforms();

    void setupFBO();
    void pass1();
    void pass2();
    void pass3();

    void drawScene(); // Normal Scene
    void drawNoise(); // Noise
    float gauss(float, float);

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
