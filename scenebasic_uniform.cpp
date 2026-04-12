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

    // * Textures *
    baseTex = Texture::loadTexture("media/toilet/source/Toilet_BaseColor.png"); // BaseColor 
    normalMap = Texture::loadTexture("media/toilet/source/Toilet_NormalOGL8.png"); // NormalMap Texture

    // HDR/Bloom Setup
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

    // LumThresh uniform
    prog.setUniform("LumThresh", 1.7f);

    // Compute and sum the weights
    float weights[10], sum, sigma2 = 25.0f;
    weights[0] = gauss(0, sigma2);
    sum = weights[0];
    for (int i = 1; i < 10; i++) {
        weights[i] = gauss(float(i), sigma2);
        sum += 2 * weights[i];
    }
    // Normalize the weights and set the uniform
    for (int i = 0; i < 10; i++) {
        std::stringstream uniName;
        uniName << "Weight[" << i << "]";
        float val = weights[i] / sum;
        prog.setUniform(uniName.str().c_str(), val);
    }

    // Set up two sampler objects for linear and nearest filtering
    GLuint samplers[2];
    glGenSamplers(2, samplers);
    linearSampler = samplers[0];
    nearestSampler = samplers[1];
    GLfloat border[] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Set up the nearest sampler
    glSamplerParameteri(nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(nearestSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameterfv(nearestSampler, GL_TEXTURE_BORDER_COLOR, border);

    // Set up the linear sampler
    glSamplerParameteri(linearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(linearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(linearSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(linearSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glSamplerParameterfv(linearSampler, GL_TEXTURE_BORDER_COLOR, border);

    // We want nearest sampling except for the last pass.
    glBindSampler(2, nearestSampler);
    glBindSampler(3, nearestSampler);
    glBindSampler(4, nearestSampler);
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
    pass1();
    computeLogAveLuminance();
    pass2();
    pass3();
    pass4();
    pass5();
}

// Has all parameters and function calls to render main meshes onto scene (Toilet)
void SceneBasic_Uniform::drawScene()
{
    // Set material properties (Specular & Shininess)
    prog.setUniform("Material.Ks", vec3(0.5f, 0.5f, 0.5f));
    prog.setUniform("Material.Shininess", 180.0f);

    // * Light *
    // Light Intensity
    prog.setUniform("Light[0].L", vec3(1.0f, 1.0f, 1.0f));
    prog.setUniform("Light[1].L", vec3(0.05f, 0.0f, 0.0f));
    prog.setUniform("Light[2].L", vec3(0.0f, 0.0f, 0.4f));

    // Light Ambient
    prog.setUniform("Light[0].La", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Light[1].La", vec3(0.05f, 0.0f, 0.0f));
    prog.setUniform("Light[2].La", vec3(0.0f, 0.0f, 0.05f));

    // Fog Properties
    prog.setUniform("Fog.MaxDist", 2.0f);
    prog.setUniform("Fog.MinDist", 1.0f);
    prog.setUniform("Fog.Color", vec3(color[0], color[1], color[2]));

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

void SceneBasic_Uniform::pass1()
{
    prog.use();
    prog.setUniform("Pass", 1);

    // glClearColor(color[0], color[1], color[2], color[3]);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // * Texture Units * 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseTex); // BaseColor 
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap); // NormalMap Texture

    // Camera view
    view = glm::lookAt(vec3(0.5f, 0.75f, 0.75f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);

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

    // Fog Properties
    prog.setUniform("Fog.MaxDist", 2.0f);
    prog.setUniform("Fog.MinDist", 1.0f);
    prog.setUniform("Fog.Color", vec3(color[0], color[1], color[2]));

    drawScene();
}

void SceneBasic_Uniform::pass2()
{
    prog.use();
    prog.setUniform("Pass", 2);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);

    // Writing to tex1 this time
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);
    glViewport(0, 0, bloomBufWidth, bloomBufHeight);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    model = mat4(1.0);
    view = mat4(1.0);
    projection = mat4(1.0);
    setMatrices();

    // Render the full-screen quad
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass3()
{
    prog.use();
    prog.setUniform("Pass", 3);

    // Writing to tex2 this time
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);

    // Render full-screen quad
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass4()
{
    prog.use();
    prog.setUniform("Pass", 4);

    // Writing to tex1 this time
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);

    // Render full-screen quad
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void SceneBasic_Uniform::pass5()
{
    prog.use();
    prog.setUniform("Pass", 5);

    // Bind to the default framebuffer (This is what actually draws in the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);

    // In this pass, we're reading from text 3 (unit 3) and we want linear sampling to get an extra blur
    glBindSampler(3, linearSampler);

    // Render full-screen quad
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Revert to nearest sampling
    glBindSampler(3, nearestSampler);
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

// Sets up the fbo for rendering to a texture
void SceneBasic_Uniform::setupFBO() {

    // Generate and bind the framebuffer
    glGenFramebuffers(1, &hdrFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFbo);

    // Create the texture object (unit 2)
    glGenTextures(1, &hdrTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, width, height);

    // Bind the texture to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrTex, 0);

    // Create the depth buffer
    GLuint depthBuf;
    glGenRenderbuffers(1, &depthBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    // Bind the depth buffer to the FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

    // Set the targets for the fragment output variables
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    // Create an FBO for the bright-pass filter and blur
    glGenFramebuffers(1, &blurFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, blurFbo);

    // Create two texture objects to ping-pong for the bright-pass filter
    // and the two-pass blur
    bloomBufWidth = width / 8;
    bloomBufHeight = height / 8;

    // Unit 3
    glGenTextures(1, &tex1);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex1);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, bloomBufWidth, bloomBufHeight);

    // Unit 4
    glGenTextures(1, &tex2);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, bloomBufWidth, bloomBufHeight);

    // Bind tex1 to the FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex1, 0);
    glDrawBuffers(1, drawBuffers);

    // Unbind the framebuffer, and revert to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::computeLogAveLuminance()
{
    int size = width * height;
    std::vector<GLfloat> texData(size * 3);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, texData.data());

    float sum = 0.0f;

    for (int i = 0; i < size; i++) {

        float lum = glm::dot(vec3(texData[i * 3 + 0], texData[i * 3 + 1], texData[i * 3 + 2]), vec3(0.2126f, 0.7152f, 0.0722f));
        sum += logf(lum + 0.00001f);
    }

    prog.use();
    prog.setUniform("AveLum", expf(sum / size));
}

float SceneBasic_Uniform::gauss(float x, float sigma2)
{
    double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
    double exponent = -(x * x) / (2.0 * sigma2);

    return (float)(coeff * exp(exponent));
}