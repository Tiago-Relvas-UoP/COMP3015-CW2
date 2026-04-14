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
#include <GLFW/glfw3.h>
#include "helper/noisetex.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

float color[4] = { 0.1f, 0.1f, 0.1f, 1.0f }; // Background/Fog Color - Currently Dark Grey

SceneBasic_Uniform::SceneBasic_Uniform() : 
    tPrev(0), angle(90.0f), rotSpeed(glm::pi<float>()/0.7f), blurEnabled(true), timeSincePress(1.0f)
{
        mesh = ObjMesh::load("media/toilet/source/Toilet.obj", false, true);
}

void SceneBasic_Uniform::initScene()
{
    // Compile shaders, Enable Depth & Set Background Color
    compile();
    glEnable(GL_DEPTH_TEST); 
    glClearColor(color[0], color[1], color[2], color[3]);

    setupFBO();

    // Array for full-screen quad
    GLfloat verts[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
    };

    GLfloat tc[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
    };

    // Full-screen quad for blur passes

    // Set up the buffers
    unsigned int handle[2];
    glGenBuffers(2, handle);
    glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), tc, GL_STATIC_DRAW);

    // Setup the vertex array object
    glGenVertexArrays(1, &fsQuad);
    glBindVertexArray(fsQuad);

    glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
    glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0); // Vertex Position

    glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
    glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2); // Texture Coordinates

    glBindVertexArray(0);

    // End of Full-screen quad for blur passes

    // Full-screen quad for noise overlay
    
    // Set up the buffers
    unsigned int noiseHandle[2];
    glGenBuffers(2, noiseHandle);

    glBindBuffer(GL_ARRAY_BUFFER, noiseHandle[0]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, noiseHandle[1]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), tc, GL_STATIC_DRAW);

    // Set up the vertex array object
    glGenVertexArrays(1, &noiseQuad);
    glBindVertexArray(noiseQuad);

    glBindBuffer(GL_ARRAY_BUFFER, noiseHandle[0]);
    glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)NULL + (0)));
    glEnableVertexAttribArray(0); // Vertex position

    glBindBuffer(GL_ARRAY_BUFFER, noiseHandle[1]);
    glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, ((GLubyte*)NULL + (0)));
    glEnableVertexAttribArray(2); // Texture coordinates

    glBindVertexArray(0);

    // End of Full-screen quad for noise overlay

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

    // Note to self: This took me hours to debug. Declare GLuint textures first before activating/binding them, otherwise there will be
    // issues with textures either not loading, or overlapping. 

    // * Textures *
    GLuint baseTex = Texture::loadTexture("media/toilet/source/Toilet_BaseColor.png"); // BaseColor 
    GLuint normalMap = Texture::loadTexture("media/toilet/source/Toilet_NormalOGL8.png"); // NormalMap Texture
    GLuint noiseTex = NoiseTex::generate2DTex(6.0f);

    // * Texture Units * 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseTex); // BaseColor 
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap); // NormalMap Texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, noiseTex);

    // Fog Properties
    prog.setUniform("Fog.MaxDist", 2.0f);
    prog.setUniform("Fog.MinDist", 1.0f);
    prog.setUniform("Fog.Color", vec3(color[0], color[1], color[2]));

    float weights[5], sum, sigma2 = 8.0f;

    // Compute and sum the weights
    weights[0] = gauss(0, sigma2);
    sum = weights[0];

    // Normalize the weights and set the uniforms
    for (int i = 1; i < 5; i++)
    {
        weights[i] = gauss(float(i), sigma2);
        sum += 2 * weights[i];
    }

    // Normalize the weights and set the uniforms
    for (int i = 0; i < 5; i++)
    {
        std::stringstream uniName;
        uniName << "Weight[" << i << "]";

        float val = weights[i] / sum;
        prog.setUniform(uniName.str().c_str(), val);
    }

    noiseProg.use();
    noiseProg.setUniform("NoiseTex", 3);
    noiseProg.setUniform("GlobalAlpha", 0.4f);

    prog.use();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Compiles the vertex and fragment shaders, and then links it to the program for use.
void SceneBasic_Uniform::compile()
{
	try {
		prog.compileShader("shader/basic_uniform.vert");
		prog.compileShader("shader/basic_uniform.frag");
		prog.link();
		prog.use();     

        noiseProg.compileShader("shader/noise.vert");
        noiseProg.compileShader("shader/noise.frag");
        noiseProg.link();
	} catch (GLSLProgramException &e) { // Catches any exceptions/errors, and then displays message on console for debug
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update( float t )
{
    float deltaT = t - tPrev; // Delta Time (Current time - Previous time)
    float keyPressCD = 0.3f;
    timeSincePress += deltaT;

    if (tPrev == 0.0f) deltaT = 0.0f;

    tPrev = t;
    angle += 0.1f * deltaT; // Update angle rotation based on delta time

    if (this->m_animate) 
    {
        angle += rotSpeed * deltaT;
        if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
    }

    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_Q) == GLFW_PRESS && timeSincePress > keyPressCD)
    {
        timeSincePress = 0.0f;
        blurEnabled = !blurEnabled;
    }
}

// Responsible for handling the scene rendering. Currently only calls drawScene() which renders toilet instances.
void SceneBasic_Uniform::render()
{

    if (blurEnabled)
    {
        pass1();
        pass2();
        pass3();
    }
    else
    {
        prog.use();
        prog.setUniform("Pass", 1);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawScene();
    }

    drawNoise();

}

void SceneBasic_Uniform::pass1()
{
    prog.use();
    prog.setUniform("Pass", 1);
    glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawScene();
}

void SceneBasic_Uniform::pass2()
{
    prog.use();

    // Set Uniform for second pass
    prog.setUniform("Pass", 2);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up the matrices
    model = mat4(1.0f);
    view = mat4(1.0f);
    projection = mat4(1.0f);
    setMatrices(1);

    // Render the full-screen quad;
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass3()
{
    prog.use();

    // Set Uniform for second pass
    prog.setUniform("Pass", 3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, intermediateTex);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up the matrices
    model = mat4(1.0f);
    view = mat4(1.0f);
    projection = mat4(1.0f);
    setMatrices(1);

    // Render the full-screen quad;
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Has all parameters and function calls to render main meshes onto scene (Toilet)
void SceneBasic_Uniform::drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera view
    view = glm::lookAt(vec3(0.5f, 0.75f, 0.75f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = glm::perspective(glm::radians(60.0f), (float)width / height, 0.3f, 100.0f);

    // Set material properties (Specular & Shininess)
    prog.setUniform("Material.Ks", vec3(0.5f, 0.5f, 0.5f));
    prog.setUniform("Material.Shininess", 300.0f);

    // Renders multiple instances of the toilet mesh, with each interation increasing the distance to test Fog Feature.
    float dist = 0.0f;
    for (int i = 0; i < 8; i++)
    {
        model = mat4(1.0f);
        model = glm::translate(model, vec3(0.0f, 0.0f, -dist));
        model = glm::rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
        model = scale(model, vec3(0.005f, 0.005f, 0.005f));
        setMatrices(1);
        mesh->render();
        dist += 0.7f;
    }
}

void SceneBasic_Uniform::drawNoise()
{
    noiseProg.use();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    model = mat4(1.0f);
	view = mat4(1.0f);
    projection = mat4(1.0f);
    setMatrices(0);

    glBindVertexArray(noiseQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glEnable(GL_DEPTH_TEST);
    prog.use();
}

float SceneBasic_Uniform::gauss(float x, float sigma2)
{
    double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
    double expon = -(x * x) / (2.0 * sigma2);
    return (float)(coeff * exp(expon));
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);

    width = w;
    height = h;

    projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
}

void SceneBasic_Uniform::setMatrices(int type)
{
    mat4 mv = view * model;

    if (type == 1)
    {
        prog.setUniform("ModelViewMatrix", mv);
        prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));
        prog.setUniform("MVP", projection * mv);
    } else if (type == 0)
    {
        noiseProg.setUniform("MVP", projection * mv);
	}
}

void SceneBasic_Uniform::setupFBO()
{
    // Generate and bind the framebuffer
    glGenFramebuffers(1, &renderFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);

    // Create the Texture Object
    glGenTextures(1, &renderTex);
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // Bind the texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

    // Create the depth buffer
    GLuint depthBuf;
    glGenRenderbuffers(1, &depthBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    // Bind the depth buffer to the FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

    // Set the targets for the fragment output variables.
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    // For debugging 
    /*GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is complete." << std::endl;
    }
    else {
        std::cout << "Framebuffer error: " << result << std::endl;
    }*/

    // Unbind the framebuffer, and revert to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // *********************
    // ** IntermediateFBO ** 
    // *********************

    // Generate and bind the framebuffer
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

    // Create the Texture Object
    glGenTextures(1, &intermediateTex);
    glActiveTexture(GL_TEXTURE0); // Use texture unit 0
    glBindTexture(GL_TEXTURE_2D, intermediateTex);

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // Bind the texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediateTex, 0);

    glDrawBuffers(1, drawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}