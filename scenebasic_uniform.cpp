#include "scenebasic_uniform.h"

#include <cstdio>
#include <cstdlib>

#include <string>
using std::string;

#include<sstream>
#include <iostream>
using std::cerr;
using std::endl;

#include "helper/glutils.h"
#include "helper/texture.h"

#include <glm/gtc/matrix_transform.hpp>

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

float color[4] = { 0.1f, 0.1f, 0.1f, 1.0f }; // Background/Fog Color - Currently Dark Grey

SceneBasic_Uniform::SceneBasic_Uniform() : 
    tPrev(0), angle(90.0f), rotSpeed(glm::pi<float>()/0.7f)
{
        mesh = ObjMesh::load("media/toilet/source/Toilet.obj", false, true);
}

void SceneBasic_Uniform::initScene()
{
    // Compile shaders, Enable Depth & Set Background Color
    compile();
    glEnable(GL_DEPTH_TEST); 
    glClearColor(color[0], color[1], color[2], color[3]);

    // Camera view
    view = glm::lookAt(vec3(0.5f, 0.75f, 0.75f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = mat4(1.0f);

    // Multiple Lights 
    float x, z;
    for (int i = 0; i < 3; i++)
    {
        std::stringstream name;
        name << "Light[" << i << "].Position";

        x = 2.0f * cosf((glm::two_pi<float>() / 3) * i);
        z = 2.0f * sinf((glm::two_pi<float>() / 3) * i);

        prog.setUniform(name.str().c_str(), view * glm::vec4(x, 1.2f, z + 1.0f, 1.0f));

    }

    // * Light *
    // Light Intensity
    prog.setUniform("Light[0].L", vec3(1.0f, 1.0f, 1.0f));
    prog.setUniform("Light[1].L", vec3(0.05f, 0.0f, 0.0f));
    prog.setUniform("Light[2].L", vec3(0.0f, 0.0f, 0.4f));

    // Light Ambient
    prog.setUniform("Light[0].La", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Light[1].La", vec3(0.05f, 0.0f, 0.0f));
    prog.setUniform("Light[2].La", vec3(0.0f, 0.0f, 0.05f));

    // * Textures *
    GLuint baseTex = Texture::loadTexture("media/toilet/source/Toilet_BaseColor.png"); // BaseColor 
    GLuint normalMap = Texture::loadTexture("media/toilet/source/Toilet_NormalOGL8.png"); // NormalMap Texture

    // * Texture Units * 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseTex); // BaseColor 
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap); // NormalMap Texture


    // Fog Properties
    prog.setUniform("Fog.MaxDist", 2.0f);
    prog.setUniform("Fog.MinDist", 1.0f);
    prog.setUniform("Fog.Color", vec3(color[0], color[1], color[2]));

}

// Compiles the vertex and fragment shaders, and then links it to the program for use.
void SceneBasic_Uniform::compile()
{
	try {
		prog.compileShader("shader/basic_uniform.vert");
		prog.compileShader("shader/basic_uniform.frag");
		prog.link();
		prog.use();       
	} catch (GLSLProgramException &e) { // Catches any exceptions/errors, and then displays message on console for debug
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update( float t )
{
    float deltaT = t - tPrev; // Delta Time (Current time - Previous time)

    if (tPrev == 0.0f) deltaT = 0.0f;

    tPrev = t;
    angle += 0.1f * deltaT; // Update angle rotation based on delta time

    if (this->m_animate) 
    {
        angle += rotSpeed * deltaT;
        if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
    }
}

// Responsible for handling the scene rendering. Currently only calls drawScene() which renders toilet instances.
void SceneBasic_Uniform::render()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    drawScene();
}

// Has all parameters and function calls to render main meshes onto scene (Toilet)
void SceneBasic_Uniform::drawScene()
{
    // Set material properties (Specular & Shininess)
    prog.setUniform("Material.Ks", vec3(0.5f, 0.5f, 0.5f));
    prog.setUniform("Material.Shininess", 180.0f);

    // Renders multiple instances of the toilet mesh, with each interation increasing the distance to test Fog Feature.
    float dist = 0.0f;
    for (int i = 0; i < 8; i++)
    {
        model = mat4(1.0f);
        model = glm::translate(model, vec3(0.0f, 0.0f, -dist));
        model = glm::rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
        model = scale(model, vec3(0.005f, 0.005f, 0.005f));
        setMatrices();
        mesh->render();
        dist += 0.7f;
    }
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);

    width = w;
    height = h;

    projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
}

void SceneBasic_Uniform::setMatrices()
{
    mat4 mv = view * model;

    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
    prog.setUniform("MVP", projection * mv);
}