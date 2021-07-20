#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "tools/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "tools/stb_image_write.h"

#include "SHLighting.h"

SHLighting::SHLighting()
{
    width = 800;
    height = 600;

    title = "SHLighting";

    camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
    lastX = width / 2.0f;
    lastY = height / 2.0f;
    firstMouse = true;

    deltaTime = 0.0f;
    lastFrame = 0.0f;
}

void SHLighting::setWindowSize(unsigned int width_, unsigned int height_)
{
	width = width_;
	height = height_;
}

void SHLighting::setWindowTitle(const std::string& str)
{
	title = str;
}

void SHLighting::save_sh_texture()
{
    harmonics m_harmonics;
    texture text("assets/cal_light.jpg");

    m_harmonics.set_sh_order(4);
    m_harmonics.set_sample_num(100);

    m_harmonics.SH_setup_spherical_samples();
    m_harmonics.generate_coefficients(texture::cal_test_light, text); 

    m_harmonics.print_coeff();

    save_vector_txt("assets/cal_light_coeff.txt", m_harmonics.sh_coefficents);

    unsigned char* sh_texture_data = new unsigned char[text.width * text.height * text.channel];

    int index = 0;
    for (int j = 100 - 1; j >= 0; j--)
    {
        for (int i = 0; i < 100; i++)
        {
            double theta = (double)j / 100 * PI;
            double phi = (double)i / 100 * 2 * PI;

            double x = sin(theta) * cos(phi);
            double y = sin(theta) * sin(phi);
            double z = cos(theta);

            std::vector<double> Y;
            m_harmonics.generate_basis(x, y, z, Y);

            double f_r = 0, f_g = 0, f_b = 0;

            for (int l = 0; l < Y.size(); l++)
            {
                f_r += m_harmonics.sh_coefficents[l][0] * Y[l];
                f_g += m_harmonics.sh_coefficents[l][1] * Y[l];
                f_b += m_harmonics.sh_coefficents[l][2] * Y[l];
            }

            sh_texture_data[index++] = static_cast<unsigned char>(f_r * 255);
            sh_texture_data[index++] = static_cast<unsigned char>(f_g * 255);
            sh_texture_data[index++] = static_cast<unsigned char>(f_b * 255);
        }
    }

    text.write_to_jpg("assets/cal_light_sh_4.jpg", 100, 100, 3, sh_texture_data);

    delete[] sh_texture_data;

}

void SHLighting::run()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (window == NULL)
    {
        std::cout << "failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    auto framebuffersizecallback = [](GLFWwindow* w, int width, int height)
    {
        static_cast<SHLighting*>(glfwGetWindowUserPointer(w))->framebuffer_size_callback(w, width, height);
    };
    auto mousecallback = [](GLFWwindow* w, double xpos, double ypos)
    {
        static_cast<SHLighting*>(glfwGetWindowUserPointer(w))->mouse_callback(w, xpos, ypos);
    };
    auto scrollcallback = [](GLFWwindow* w, double xoffset, double yoffset)
    {
        static_cast<SHLighting*>(glfwGetWindowUserPointer(w))->scroll_callback(w, xoffset, yoffset);
    };

    glfwSetFramebufferSizeCallback(window, framebuffersizecallback);
    glfwSetCursorPosCallback(window, mousecallback);
    glfwSetScrollCallback(window, scrollcallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    glEnable(GL_DEPTH_TEST);

#pragma region ball_mesh

    int lat = 40, lon = 81;
    int ball_vertex_num = lat * lon + 2;
    int ball_tri_num = lon * lat * 2;
    double radius = 2.0;

    //generate_ball_mesh("assets/ball_mesh.txt", lat, lon, radius);

    float* ball_mesh_vertex = new float[ball_vertex_num * 8];
    unsigned int* ball_vertex_index = new unsigned int[ball_tri_num * 3];

    load_mesh("assets/ball_mesh.txt", ball_mesh_vertex, ball_vertex_index);

    unsigned int ball_VAO, ball_VBO, ball_EBO;
    glGenVertexArrays(1, &ball_VAO);
    
    glGenBuffers(1, &ball_VBO);
    glGenBuffers(1, &ball_EBO);

    glBindVertexArray(ball_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, ball_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ball_vertex_num * 8, ball_mesh_vertex, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ball_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * ball_tri_num * 3, ball_vertex_index, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

#pragma endregion

#pragma region earthmap_texture

    unsigned int earthmap_texture;
    glGenTextures(1, &earthmap_texture);

    glBindTexture(GL_TEXTURE_2D, earthmap_texture);
    //set wrap and filter mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    texture earth_map("assets/earthmap.jpg");

    if (earth_map.texture_data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, earth_map.width, earth_map.height, 0, GL_RGB, GL_UNSIGNED_BYTE, earth_map.texture_data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texutre" << std::endl;
    }

#pragma endregion

#pragma region skybox

    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };


    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    std::vector<std::string> faces = { "assets/skybox/right.jpg",
                                       "assets/skybox/left.jpg",
                                       "assets/skybox/top.jpg", 
                                       "assets/skybox/bottom.jpg", 
                                       "assets/skybox/front.jpg", 
                                       "assets/skybox/back.jpg" };

    unsigned int skyboxID;
    glGenTextures(1, &skyboxID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);
    
    int skybox_width, skybox_height, skybox_channel;
    for (int i = 0; i < faces.size(); i++)
    {
        unsigned char* skybox_data = stbi_load(faces[i].c_str(), &skybox_width, &skybox_height, &skybox_channel, 0);
        if (skybox_data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, skybox_width, skybox_height, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data);
            stbi_image_free(skybox_data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(skybox_data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

#pragma endregion

#pragma region SH_Coeff


    int tex_w = 512, tex_h = 512;
    unsigned int tex_output;

    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);

    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    Shader SHCoeffShader("shaders/SHCoeff_comp.cs");

    SHCoeffShader.use();
    
    int work_grp_cnt[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    
    glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::unique_ptr<float[]> pixels(new float[tex_w * tex_h * 4]);

    glActiveTexture(GL_TEXTURE0);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.get());

    unsigned char* m_image = new unsigned char[tex_w * tex_h * 4];

    for (int i = 1; i <= tex_w * tex_h * 4; i++)
    {
        m_image[i - 1] = static_cast<unsigned char>(pixels[i - 1] * 255);
    }


    stbi_write_jpg("assets/compute_test.jpg", tex_w, tex_h, 4, m_image, 50);

#pragma endregion



    Shader ballShader("shaders/ball_vert.vs", "shaders/ball_frag.fs");

    Shader calLightShader("shaders/cal_light_vert.vs", "shaders/cal_light_frag.fs");

    Shader skyboxShader("shaders/skybox_vert.vs", "shaders/skybox_frag.fs");

    ballShader.use();
    ballShader.setInt("skybox", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    camera.setInitialStatus(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        ballShader.use();
        
        glm::mat4 model = glm::mat4(1.0);
        model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
        ballShader.setMat4("model", model);

        glm::mat4 view = camera.GetViewMatrix();
        ballShader.setMat4("view", view);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width / (float)height, 0.1f, 100.0f);
        ballShader.setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxID);

        glBindVertexArray(ball_VAO);
        glDrawElements(GL_TRIANGLES, ball_tri_num * 3, GL_UNSIGNED_INT, 0);

        calLightShader.use();

        model = glm::translate(model, glm::vec3(5.0f, 0.0f, 0.0f));
        calLightShader.setMat4("model", model);

        calLightShader.setMat4("view", view);

        calLightShader.setMat4("projection", projection);

        glDrawElements(GL_TRIANGLES, ball_tri_num * 3, GL_UNSIGNED_INT, 0);

        //skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxID);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        glDepthFunc(GL_LESS);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &ball_VAO);
    glDeleteBuffers(1, &ball_VBO);
    glDeleteBuffers(1, &ball_EBO);

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();

}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void SHLighting::processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void SHLighting::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void SHLighting::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void SHLighting::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}