#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"
#include "helper/objmesh.h"
#include "helper/skybox.h"
#include "helper/glslprogram.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "irrklang.h"

class SceneBasic_Uniform : public Scene
{
private:
    // GLSL programs
    GLSLProgram prog; // Main 
    GLSLProgram noiseProg; // Noise 
    GLSLProgram skyboxProg; // Skybox

    // Full-screen quad + FBO/Texture resources for Guassian Blur
    GLuint fsQuad;
    GLuint renderFBO, intermediateFBO;
    GLuint renderTex, intermediateTex;

    // Noise Overlay Quad + Texture for procedural noise
    GLuint noiseQuad;
    GLuint noiseTex;

    // Scene resources
    SkyBox skybox;
    std::unique_ptr<ObjMesh> mesh;

    // Animation and Effect Parameters
    float tPrev, angle, rotSpeed;
    bool blurEnabled; 
	float timeSincePress;

    // Audio
    irrklang::ISoundEngine* soundEngine;
    float volume;

    // Main method helpers
    void compile();
    void setMatrices(int type); 
    void setTextures();
    void setupQuadBuffers();
    void setupUniforms();

    // FBO / Pass helpers for Gaussian Blur
    void setupFBO();
    void pass1();
    void pass2();
    void pass3();

    // Render helpers
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
