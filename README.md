# COMP3015-CW2: Polish Toilets In Space
Note: This is an extension of CW1, which can be found [here](https://github.com/Tiago-Relvas-UoP/COMP3015-CW1/tree/main)

## YOUTUBE VIDEO
The youtube video, showcasing the project, can be found [here](https://youtu.be/JGnGIBx2xK8)

## PROJECT DESCRIPTION
"Polish Toilets in Space" is a 3D Scene consisting of a few toilets spinning in space.

### Features
- Toilet meshes that uses a Base Color and a Normal Map.
- Multiple Lights using Blinn-Phong Shading
- Applied Toon Shading modifier to the diffuse component of B.P. Shading
- Fog Factor.
- Skybox using a HDR Cube Map
- Toggleable Gaussian Blur (Press Q to Toggle On/Off)
- 2D Procedurally-Generated Noise that overlays the scene.

### SPECIFICATION/DEPENDENCIES
- IDE used: Visual Studio Community 2022
- OS: Windows 11
- Dependencies
	- The following libraries are used: GLAD, GLFW, GLM, KHR, irrKlang and some files from learnopengl.com (stb_image.cpp, stb_image.h, stb_image_write.h, mesh.h, model.h, shader.h, shader_m.h).
- Configuration Properties
	- VC++ Directories
		-Include Directories: C:\Users\Public\OpenGL\include
		- Library Directories: C:\Users\Public\OpenGL\lib
	- Linker
		- Input
		- Additional Dependencies: glfw3.lib & opengl32.lib
 
### HOW-TO RUN
- From source code:
	- Download Visual Studio Community 2022 & all required libraries not included in the project directory (Consult "Configuration property". These must be in the specified directories aswell)
	- Download source code .zip
 	- Extract .zip contents to separate folder
  	- run .sln file
  	- In the upper section, choose appropriate Debugger settings (x64/x86 and Debug/Release).
  	- Click "Local Windows Debugger" to run the scene.
  	  
- From executable:
	- Download executable from release section in this repository
	- Extract .zip contents into a separate folder
 	- Click the ".exe" file to run the scene. 

## What makes this Shader Program Special? (+ Inspirations)
This program is heavily inspired by a random video I stumbled upon on youtube consiting of a Glowing Toilet thats indefinitely spinning whilst a Polish music plays in the background. With this discovery, I decided to attempt to replicate the feeling given by that video but with my own original touches, using certain techniques to improve the scene (Such as addition of a Skybox, noise and Toon Shading).

Source material:
- [Spinning Toilet Video](https://youtu.be/s0Vsfc3C35U?si=dxjRUz7hcequReph)
- [Polish Music](https://www.youtube.com/watch?v=3VizXab7XE0) (Po twojej pysznej zupie - Kamil)

# Program navigation (How everything ties together)

## Scenebasic_uniform.h
Defines and stores all essential resources/parameters to be used in the program. This includes all GLSL (Shader) programs, GLUINTs, Animation/Effect Parameters, Scene Resources and Helper methods.

## Scenebasic_uniform.cpp
Responsible for bringing the program to life. It is divided into multiple methods:

### Scenebasic_Uniform()
Acts as the constructors. All parameters are defined here, aswell as the Toilet Mesh location within the Project Directory.

### InitScene()
Responsible for initializing the scene. Uses multiple helper methods (SetupFBO, SetupQuadBuffers, setupUniforms() and setTextures), aswell as enabling Depth Testing and Alpha Blending (Which allows Noise to be rendered alongside the normal scene). Additionally, it starts playthrough of the background music using a Sound Engine provided by the `irrKlang` Library. 

```cpp
void SceneBasic_Uniform::initScene()
{
    // Compile shaders.
    compile();
    glEnable(GL_DEPTH_TEST); // Enable Depth

    // Set up resources (FBO, Quad Buffers, Uniforms, Textures)
    setupFBO();
    setupQuadBuffers();
    setupUniforms();
    setTextures();

    // Switch to main shader program
    prog.use();

    // Start background music (In 2D), with looping enabled and volume defined by its respective parameter
    soundEngine->play2D("media/music/polishtoiletost.wav", true);
    soundEngine->setSoundVolume(volume);

    // Enable Alpha Blending for Overlay (Applies to Noise Texture, so it renders on top of scene)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
```

#### compile()
Responsible for compiling and linking the appropriate shader files (Vertex & Fragment Shaders) into their respective GLSL programs. There are three distinctive programs: prog (Main Shaders), noiseProg (Noise Shaders) and skyboxProg (Skybox Shaders). An exception handler is also in-place in-case of linking errors.

```cpp
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

        skyboxProg.compileShader("shader/skybox.vert");
        skyboxProg.compileShader("shader/skybox.frag");
        skyboxProg.link();
	} catch (GLSLProgramException &e) { // Catches any exceptions/errors, and then displays message on console for debug
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}
```

#### setupFBO()
This method is responsible for initializing the framebugger objects and associated textures to be used for Guassian Blur. This allows it to be rendered offscreen by doing it on a separate texture so it's not directly applied to the screen, so this post-processing technique is applied based on image-data.

```cpp
// Creates framebuffers and textures that are used for multi-pass rendering for the Gaussian Blur, when enabled. (Provided from the labs)
void SceneBasic_Uniform::setupFBO()
{
    // *********************
    // ***** RenderFBO ***** 
    // *********************

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
```

### setupQuadBuffers()
This method is responsible for initializing the Vertex Buffer Objects (VBOs) and Vertex Array Objects (VAO) required to render full-screen quads, which are used for overlay effects (fsQuad handles Gauss Blur, and noiseQuad handles noise). Verts define the coordinates for two triangles that conver the entire screen (ranges from -1.0 to 1.0, whilst tc defines the texture coordinates (ranges from 0.0 to 1.0).

```cpp
// Responsible for setting up the VBO and VAO for full-screen quads (Used for Gaussian Blur, and Noise Overlay).
void SceneBasic_Uniform::setupQuadBuffers()
{
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
}
```

#### setupUniforms()
Responsible for setting all appropriate uniform parameters to their respective shader programs. This includes the following:

- Multiple-Light Setup, and Intensity/Ambient parameters assigned to each (Program uses 3 Different Light Sources)
- Toilet Material parameters (Specular and Shininess).
- Fog Clipping parameters (Max. and Min. Distance).
- Noise Texture Uniform assignment for generation, and Global Alpha parameters (The latter defines "Transparency" in scene).
- Weight parameter setup for Gauss Blur (Computes and sums the weights, aswell as normalizing and finally setting the values in its respective shader program).

```cpp
// Responsible for setting up all shader uniforms, for all shader programs (Includes properties for Lights, Fog, Material, Gaussian Blur Weights, and Noise Overlay).
void SceneBasic_Uniform::setupUniforms()
{
    // ** Noise Uniforms **
    noiseProg.use(); // Switch to noise shader program
    noiseProg.setUniform("NoiseTex", 3); // Texture assigning (Unit 3)
    noiseProg.setUniform("GlobalAlpha", 0.2f); // Alpha value for noise overlay (transparency)

    // Switch to main shader program
    prog.use();

    // Set-up for Multiple Blinn-Phong Lights
    float x, z;
    for (int i = 0; i < 3; i++)
    {
        std::stringstream name;
        name << "Light[" << i << "].Position";

        x = 2.0f * cosf((glm::two_pi<float>() / 3) * i);
        z = 2.0f * sinf((glm::two_pi<float>() / 3) * i);

        prog.setUniform(name.str().c_str(), view * glm::vec4(x, 1.2f, z + 1.0f, 1.0f));

    }

    // ** Main Shader Uniforms **
    
    // Light Intensity
    prog.setUniform("Light[0].L", vec3(1.0f, 1.0f, 1.0f));
    prog.setUniform("Light[1].L", vec3(0.2f, 0.0f, 0.0f));
    prog.setUniform("Light[2].L", vec3(0.0f, 0.0f, 0.5f));

    // Light Ambient
    prog.setUniform("Light[0].La", vec3(0.6f, 0.6f, 0.6f));
    prog.setUniform("Light[1].La", vec3(0.05f, 0.0f, 0.0f));
    prog.setUniform("Light[2].La", vec3(0.0f, 0.0f, 0.05f));

    // Material Properties
    prog.setUniform("Material.Ks", vec3(0.5f, 0.5f, 0.5f));
    prog.setUniform("Material.Shininess", 300.0f);

    // Fog Properties
    prog.setUniform("Fog.MaxDist", 2.0f);
    prog.setUniform("Fog.MinDist", 1.0f);

    // ** Gaussian Blur **

	// Gaussian Blur Weights
    float weights[5], sum, sigma2 = 1.0f;

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
}
```

#### gauss()
This method is what computes the weight for the Gauss Blur, which is called inside setupUniforms(). Output is based on a given X Offset, and Sigma2 Variance.

```cpp
// Responsible for computing Guassian Blur Weights for given offset x and variance sigma2.
float SceneBasic_Uniform::gauss(float x, float sigma2)
{
    double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
    double expon = -(x * x) / (2.0 * sigma2);
    return (float)(coeff * exp(expon));
}
```

#### setTextures()
Responsible for defining each Textures location within the project directory, and assigning each to a different texture unit:

- Base Color (Toilet): Assigned to Texture Unit 0
- Normal Map (Toilet): Assigned to Texture Unit 1
- Texture0 (Gauss): Later used as Input Texture for Render & Intermediate Textures, used for Gauss Blur Pass. Assigned to Texture Unit 2
- Procedural Noise (Noise Overlay): Assigned to Texture Unit 3
- HDR Cubemap (Skybox) Assigned to Texture Unit 4

```cpp
// Loads all Textures, and binds them to their respective texture units
void SceneBasic_Uniform::setTextures()
{
    // * Textures *
    GLuint baseTex = Texture::loadTexture("media/toilet/source/Toilet_BaseColor.png"); // Base Color (Toilet)
    GLuint normalMap = Texture::loadTexture("media/toilet/source/Toilet_NormalOGL8.png"); // Normal Map (Toilet)
    GLuint noiseTex = NoiseTex::generate2DTex(6.0f); // Procedural Noise
    GLuint cubeTex = Texture::loadHdrCubeMap("media/skybox/stars-hdr/stars"); // HDR Cubemap (Skybox)

    // * Texture Units * 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseTex); // Base Color (Unit 0)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap); // Normal Map (Unit 1)
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, noiseTex); // Procedural Noise (Unit 3)
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex); // HDR Cubemap (Unit 4)
}
```

### Update()
This method runs each frame, and takes the current time as input. It is responsible for calculation Delta-Time by taking the current time and subtract it by the previous time (Which is defined as current time before the method runs again). The angle is updated each frame based on Delta-Time, and either a low-value (0.1f) or the rotSpeed parameter, depending if the scene has animation enabled or not. This method also detects key-pressed: Pressing the `Q` Key toggles Gauss Blur, and resets a variable back to 0, which increments itself each interation based on Delta-Time, and if this value is higher than the set cooldown, then it allows the key to be pressed again (Done to prevent multiple key-presses to be detected despite only physically pressing the Q once.)

```cpp
void SceneBasic_Uniform::update( float t )
{
    float deltaT = t - tPrev; // Delta Time (Current time - Previous time)
    float keyPressCD = 0.3f; // Cooldown for Key Press
    timeSincePress += deltaT; // Increment times since key press each frame.

    if (tPrev == 0.0f) deltaT = 0.0f;

    tPrev = t;
    angle += 0.1f * deltaT; // Update angle rotation based on delta time 

    // Apply main rotation speed to angle WHEN animation is enabled
    if (this->m_animate) 
    {
        angle += rotSpeed * deltaT;
        if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
    }

    // Toggle Guassian Blur with Key Press (Q).
    if (glfwGetKey(glfwGetCurrentContext(), GLFW_KEY_Q) == GLFW_PRESS && timeSincePress > keyPressCD)
    {
        timeSincePress = 0.0f; // Reset time since press to 0, so the cooldown starts.
        blurEnabled = !blurEnabled; // inverts BlurEnabled bool value.
    }
}
```

### Render()
Responsible for continously rendering the scene. An if-statement is used to define how to render the scene, in this case based on if Gauss Blur is enabled or not. On start-up, when the Gauss is on by default, it renders the scene using three different "pass" methods (see further below). Otherwise, it renders the default scene without this applied using drawScene(), and assigning the framebuffer to the default. Regardless of how scene rendering is processed, drawNoise() is always called in the end to render the 2D Noise Overlay.

```cpp
// Responsible for handling the scene rendering.
void SceneBasic_Uniform::render()
{
    // If Blur is enabled, render scene as normal then do multiple blur-passes.
    if (blurEnabled)
    {
        pass1(); // Render main scene to texture (renderTex) using renderFBO.
		    pass2(); // Horizontal blur pass to intermediate texture (intermediateTex) using intermediateFBO.
		    pass3(); // Vertical blur pass to default framebuffer, using intermediateTex as input.
    }
    else // If blur is disabled, only render the main scene using default framebuffer.
    {
        // Switch to main shader program
        prog.use();

        // Configure shader
        prog.setUniform("Pass", 1);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Default Framebuffer
        glEnable(GL_DEPTH_TEST);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawScene(); // Draw main scene
    }

    drawNoise(); // Draw procedural generated noise as overlay on top of scene.
}
```

#### Draw Scene()
Responsible for drawing the main scene. It starts off by clearing the Color and Depth buffers, and applying the appropriate camera view and projection. Before drawing a particular element, the shader matrices are set using setMatrices() and definying a shader type (Discussed after this segment). The first thing that is drawn in the scene is the Skybox, with the model being reset back to 1.0f to ensure no irregular rendering. Afterwards, multiple Toilet Mesh instances are generating using a For Loop, with each interation increasing the render distance in order to test the fog factor, using `translate`. To handle rotation, `rotation` is declared using angle as a variable, and since Update() continously increments this value, then each render ensures a different rotation value for the toilet meshes.

```
// Draw the main 3D Scene (Skybox + Toilet Meshes).
void SceneBasic_Uniform::drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera setup
    view = glm::lookAt(vec3(0.5f, 0.75f, 0.75f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    projection = glm::perspective(glm::radians(70.0f), (float)width / height, 0.3f, 100.0f);

    // Draw Skybox
    skyboxProg.use();
    model = mat4(1.0f);
    setMatrices(2);
    skybox.render();

    // Switch to main shader program
    prog.use();

    // Draw Toilet Meshes
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
        dist += 0.7f; // Increases distance for next interation to showcase Fog Effect.
    }
}
```

#### drawNoise()
Called last, and responsible for drawing the noise onto the scene. Before rendering the Noise, Depth Testing is disabled since noise acts as 2D Overlay Effect. Matrices are reset calling setMatrices(), and afterwards `noiseQuad` is assigned as the main VAO before drawing the scene. On end, it re-enables depth testing and switchs back to the main shader program (prog).

```cpp
// Draw procedural noise overlay as full-screen quad (noiseQuad)
void SceneBasic_Uniform::drawNoise()
{
    // Switch to noise shader program
    noiseProg.use();

	// Switch to default framebuffer, and disable depth testing since noise is a 2D overlay effect.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    // Reset M.V.P., and set matrices.
    model = mat4(1.0f);
	view = mat4(1.0f);
    projection = mat4(1.0f);
    setMatrices(0);

    // // Render the full-screen quad using noiseQuad, and re-enable depth testing.
    glBindVertexArray(noiseQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);

    // Switch back to main shader program
    prog.use();
}
```

#### Blur Passes (pass1(), pass2() and pass3())
When Gauss Blur is enabled, the scene rendering goes through three different passes 

- pass1(): Calls drawScene() using the `renderFBO` framebuffer rather than the default.
  
```cpp
// Pass 1: Render scene using renderFBO as framebuffer.
void SceneBasic_Uniform::pass1()
{
    // Switch to main shader program
    prog.use();

    // Configure shader
    prog.setUniform("Pass", 1);
    glBindFramebuffer(GL_FRAMEBUFFER, renderFBO); // renderFBO framebuffer.
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawScene(); // Draw main scene
}
```

- pass2(): Draws the Vertical Blur Blur by using `intermediateFBO` as the framebuffer, and `renderTex` as the input Texture. Similiar to drawNoise, it also disables depth testing since this is a mere 2D Post-processing effect over the main scene. Before drawing, fsQuad is assigned to the VAO, so the output renders to the full-screen quad.

```cpp
// Pass 2: Render full-screen quad using intermediateFBO as framebuffer, and renderTex as input texture for horizontal blur.
void SceneBasic_Uniform::pass2()
{
    // Switch to main shader program
    prog.use();

    // Configure shader, and bind renderTex as input texture
    prog.setUniform("Pass", 2);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderTex);

    // Disable Depth Testing, since blur is a 2D Post-Processing Effect, and clear color buffer.
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

	// Reset M.V.P., and set matrices.
    model = mat4(1.0f);
    view = mat4(1.0f);
    projection = mat4(1.0f);
    setMatrices(1);

    // Render the full-screen quad;
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}
```

- pass3(): Draws the Horizontal Blur. Similiar to pass2(), however the default framebuffer is used, and `intermediateTex` is used as the input texture.

```cpp
// Render full-screen quad using default framebuffer, and intermediateTex as input texture for vertical blur.
void SceneBasic_Uniform::pass3()
{
    // Switch to main shader program
    prog.use();

    // Configure shader, and bind intermediateTex as input texture
    prog.setUniform("Pass", 3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, intermediateTex);
    glClear(GL_COLOR_BUFFER_BIT);

    // Reset M.V.P., and set matrices.
    model = mat4(1.0f);
    view = mat4(1.0f);
    projection = mat4(1.0f);
    setMatrices(1);

    // Render the full-screen quad;
    glBindVertexArray(fsQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
```

# Vertex and Fragment Shaders

## Main Shaders

### basic_uniform.frag

The fragment shader is called to handle everything related to the Toilet Meshes, including Light Shading (Blinn-Phong and afterwards Toon Shading), Blur weights, Fog and texture units.. Multiple parameters are set, which are transmitted form the `Scenebasic_uniform.cpp` file using the setupUniforms() methods, and structs are used to set-up related uniforms (Example Light takes both Intensity and Ambient).

```glsl
in vec3 LightDir[3];
in vec3 ViewDir;
in vec2 TexCoord;
in vec3 Position;

layout (binding = 0) uniform sampler2D baseTex;
layout (binding = 1) uniform sampler2D normalMap;
layout (binding = 2) uniform sampler2D Texture0;
layout (binding = 4) uniform samplerCube skyBoxTex; 

layout (location = 0) out vec4 FragColor;

uniform float EdgeThreshold;
uniform int Pass;
uniform float Weight[5];

uniform struct LightInfo
{
	vec4 Position; // Light position in eye coords.
	vec3 La; // Ambient light intensity
	vec3 L; // Diffuse and specular light intensity
} Light[3];

uniform struct MaterialInfo
{
	vec3 Ks; // Specular Reflectivity
	float Shininess; // Specular Shininess factor
} Material;

uniform struct FogInfo
{
	float MaxDist;
	float MinDist;
}Fog;

// Toon Shading Levels/ScaleFactor
const int levels = 3;
const float scaleFactor = 1.0/levels;
```

### Blinn-Phong Light Shading (vec3 blinnPhong)

Blinn-Phong Light is the main model used in this Shader, taking as input the surface normal and light number. Blinn-Phong uses Ambient, Diffuse and Specular Lighting, each determined mostly based on the mesh Base Color, apart from Ambience which takes in a custom value uniform (Light.La), and Diffuse which is modified by the Toon Shading parameters. The normalized direction (s) is calculated using the LightDir variable (inputted from Vertex Shader), and then sDotN determines how the light impacts the current surface normal. If the surface is facing light, determined by sDotN > 0, then the specular component is determined by how aligned the surface normal is with the Blinn Half-vector, which is computed based on View Vector (v) in Tangent Space, and the Light Direction (s) normalized. The output is a mix of the Ambient, Diffuse and Specular compoennts multiplied by the intensity of the current light.

```cpp
// Blinn-Phong Light Model
vec3 blinnPhong(int light, vec3 n)
{
	// Texture
	vec3 texColor = texture(baseTex, TexCoord).rgb;

	vec3 diffuse = vec3(0), spec = vec3(0); 

	vec3 ambient = Light[light].La * texColor;  // Ambient

	vec3 s = normalize(LightDir[light]); // Light Direction

	float sDotN = max(dot(s, n), 0.0);
	diffuse = texColor * floor(sDotN * levels) * scaleFactor ; // Diffuse with Toon Shading.

	if (sDotN > 0.0)
	{
		vec3 v = normalize(ViewDir); // View Vector in Tangent-space
		vec3 h = normalize(v+s); // Blinn Half-vector
		spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess); // Specular
	}

	return ambient + (diffuse + spec) * Light[light].L; // Final mix: Combines Ambient, Diffuse and Specular
}
```

This shader has three different passes, which change the FragColor output depending on the set pass uniform.

```glsl
// Main Method. Depending on current pass uniform value, the corresponding method function is called.
void main() 
{
	if (Pass == 1) FragColor = pass1();
	else if (Pass == 2) FragColor = pass2();
	else if (Pass == 3) FragColor = pass3();
}
```

- Pass 1 does multiple things: It Applies the normal mapping to the texture coordinates of the toilet mesh, giving the illusion of depth; It calculates the Fog Factors based the absolute value of the camera position (in Eye-space), and then clamping the Fog Factor parameter (which is based on this Camera Position, The maximum distance and minimum distance of fog clipping parameters), so it's minimum and maximum value is between 0 and 1 for a given mesh, and finally it calculates the shading for each Light Source.

```glsl
// Pass 1: Main Scene using Normal Mapping, Fog Factor, and Blinn-Phong.
vec4 pass1()
{
	// Normal Mapping
	vec3 norm = texture(normalMap, TexCoord).xyz;
	norm.xy = 2.0 * norm.xy - 1.0;

	// Fog calculation
	float dist = abs(Position.z);
	float fogFactor=(Fog.MaxDist - dist)/(Fog.MaxDist - Fog.MinDist);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	
	// Shading for each light source
	vec3 shadeColor = vec3(0.0);
	for (int i = 0; i < 3; i++)
	{
		shadeColor += blinnPhong(i, normalize(norm));
	}

	// Final output
	return vec4(shadeColor, fogFactor);
}
```

- Pass 2 and 3 apply a vertical and horizontal blur pass to the image, respectively. This is achieved by looking at a row/collumn of neighbour pixels, and multiplying each neighbours color by a weight value.

```glsl
// Pass 2: Vertical Gaussian Blur 
vec4 pass2()
{
	ivec2 pix = ivec2(gl_FragCoord.xy); // We grab a pixel to check if Edge

	vec4 sum = texelFetch(Texture0, pix, 0) * Weight[0];

	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 1)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -1)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 2)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -2)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 3)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -3)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, 4)) * Weight[4];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(0, -4)) * Weight[4];

	return sum;
}

// Pass 3: Horizontal Gaussian Blur
vec4 pass3()
{
	ivec2 pix = ivec2(gl_FragCoord.xy); // We grab a pixel to check if Edge

	vec4 sum = texelFetch(Texture0, pix, 0) * Weight[0];

	sum += texelFetchOffset(Texture0, pix, 0, ivec2(1, 0)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-1, 0)) * Weight[1];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(2, 0)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-2, 0)) * Weight[2];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(3, 0)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-3, 0)) * Weight[3];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(4, 0)) * Weight[4];
	sum += texelFetchOffset(Texture0, pix, 0, ivec2(-4, 0)) * Weight[4];

	return sum;
}
```

### basic_uniform.vert
The vertex shaders whole purpose is to transform the Light Direction and View Vectors from eye-space (relative to camera) to tangent-space (relative to surface), to be used for Normal Mapping. First it transforms the normal and tangent into eye space, and computes the binormal using the former. To allow the conversion from eye-space to tangent-space, a TBN (Tangent, Binormal and Normal) matrix is used that acts a coordinate system for the surface normal to use. Then, using this new matrix, each Light Position is converted to Tangent Space aswell as the View Direction. `pos` is used to store the camera-position in eye-space, which is used for Fog Calculation.

```glsl
void main()
{
	// Transform normal and tangent to eye space
	vec3 norm = normalize(NormalMatrix * VertexNormal);
	vec3 tang = normalize(NormalMatrix * vec3(VertexTangent));

	// Compute the binormal
	vec3 binormal = normalize(cross(norm, tang)) * VertexTangent.w;

	// Matrix for transformation to tangent space
	mat3 toObjectLocal = mat3(
		tang.x, binormal.x, norm.x,
		tang.y, binormal.y, norm.y,
		tang.z, binormal.z, norm.z
	);

	// Vertex position in Eye Space rather than tangent
	vec3 pos = vec3(ModelViewMatrix * vec4(VertexPosition, 1.0)).xyz;
	Position = pos; // For Fog

	// Conversion of each light position from eye to tangent space
	for (int i = 0; i < 3; i++)
	{
		LightDir[i] = toObjectLocal * (Light[i].Position.xyz - pos);
	}

	// Convered to tangent using Matrix
	ViewDir = toObjectLocal * normalize(-pos);
	TexCoord=VertexTexCoord;

	// Final output
	gl_Position = MVP * vec4(VertexPosition, 1.0);
}
```

## Noise Shaders

### noise.frag
The fragment shader for the noise program samples the noise based on a mix of two colors (SkyColor and CloudColor). To ensure a smooth transition between these two, `t` outputs a value between 0 and 1 by getting the cosin of the noises alpha channel and PI value (always returns either -1 or 1), then adding 1 on top of that and dividing by 2. The FragColor output is based on this mix, and the GlobalAlpha (which determines transparency).

```glsl
#version 460

#define PI 3.14159265

in vec2 TexCoord;
layout (location = 0) out vec4 FragColor;

uniform vec4 Color;
uniform sampler2D NoiseTex;

uniform vec4 SkyColor = vec4(0.3, 0.3, 0.9, 1.0);
uniform vec4 CloudColor = vec4(1.0, 1.0, 1.0, 1.0);

// Noise Alpha (Controls Transparency of Noise Overlay)
uniform float GlobalAlpha = 1.0;

void main() 
{
	// Sample noise
	vec4 noise = texture(NoiseTex, TexCoord);
	float t = (cos( noise.a * PI ) + 1.0) / 2.0;
	vec4 color = mix (SkyColor, CloudColor, t);

	// Output 
	FragColor = vec4( color.rgb, GlobalAlpha);
}
```

### noise.vert
The vertex shader for the noise program is simple, as all it does is pass the texture coordinates onto the fragment shader.

```glsl
#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 2) in vec2 VertexTexCoord;

out vec2 TexCoord;

uniform mat4 MVP;

void main()
{
    TexCoord = VertexTexCoord;

    // Output
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
```

## Skybox Shaders

### skybox.frag
The fragment shader for the skybox samples the hdr cubemap in eye-space, and afterwards applies Gamma Correction before the final output

```glsl
#version 460

in vec3 Vec;

layout (binding = 4) uniform samplerCube skyBoxTex; 
layout (location = 0) out vec4 FragColor;

void main() 
{
	// Sample cubemap using eye-space direction
	vec3 texColor=texture(skyBoxTex, normalize(-Vec)).rgb;
	texColor=pow(texColor, vec3(1.0/2.2)); // Gamma Correction

	// Output 
	FragColor = vec4(texColor, 1.0);
}
```

### skybox.vert
Similiar to the noise.vert shader file, it's also quite simple. All it does is pass the computed eye-space vector onto the fragment shader to be applied to the texture.

```glsl
#version 460

layout (location = 0) in vec3 VertexPosition;
	
out vec3 Vec;
uniform mat4 MVP;
uniform mat4 ModelViewMatrix;

void main()
{
	// Compute eye-space vector for cubemap sampling
	Vec = (ModelViewMatrix * vec4(VertexPosition, 0.0)).xyz;

	// Output
	gl_Position = MVP * vec4(VertexPosition, 1.0);
}
```

