# COMP3015-CW2: Polish Toilets In Space
Note: This is an extension of CW1, which can be found here [PUT LINK HERE]

## YOUTUBE VIDEO
[LINK TO YOUTUBE VIDEO HERE]

## PROJECT DESCRIPTION
"Polish Toilets in Space" is a 3D Scene consisting of a few toilets spinning in space.

### Features
- Toilet meshes that uses a Base Color and a Normal Map.
- Multiple Lights using Blinn-Phong Shading
- Applied Toon Shading modifier to the diffuse component of B.P. Shading
- Fog Factor.
- Skybox using a HDR Cube Map
- Toggleable Gaussian Blur
- 2D Procedurally-Generated Noise that overlays the scene.

### Program navigation (How everything ties together)

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
This method creates the framebuffers and textures to be used for Gaussian Blur passes, used during Render(). [NOTE TO SELF: EXPAND LATER]

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
[NOTE TO SELF: WRITE EXPLANATION HERE LATER]
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


