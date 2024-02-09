// Based on templates from learnopengl.com
#define GLEW_STATIC
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include "OrthoCamera.hpp"
#include "PerspCamera.hpp"
#include "HitRecord.hpp"
#include "Sphere.hpp"
#include "Triangle.hpp"
#include "Plane.hpp"
#include "Material.hpp"
#include "DirectionalLight.hpp"

const int objectCount = 6;
const int maxRecursionDepth = 3;
const float k_s = 0.2f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void cameraSwap(GLFWwindow *window, Camera* camera, OrthoCamera o, PerspCamera p);
void renderImage(Camera* camera, unsigned char* image, int imageWidth, int imageHeight, Surface* (&objects)[objectCount], 
                 DirectionalLight light);
glm::vec3 shade(Ray &ray, DirectionalLight &light, HitRecord& hitRecord, float &tMin, Surface** objects, int objectCount, int &recursionDepth);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

bool SWAP = false;

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "layout (location = 2) in vec2 aTexCoord;\n"    
    "out vec3 ourColor;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
	"gl_Position = vec4(aPos, 1.0);\n"
	"ourColor = aColor;\n"
	"TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 ourColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D texture1;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(texture1, TexCoord);\n"
    "}\n\0";
    

int main()
{
    int input;
    std::cout << "Press 0 for Orthographic View or 1 for Perspective View." << std::endl;
    std::cin >> input;
    std::cout << std::endl << input << " is the selected option." << std::endl;

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Display RGB Array", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // // GLEW: load all OpenGL function pointers
    glewInit();

    // build and compile the shaders
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // colors           // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
    };
    unsigned int indices[] = {  
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    // load and create a texture 
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Image Resolution
    const int imageWidth  = 1024; // keep it in powers of 2!
    const int imageHeight = 1024; // keep it in powers of 2!
    // Create the image (RGB Array) to be displayed
    unsigned char* image = new unsigned char[imageWidth*imageHeight*3];

    // glm::vec3 viewDir = glm::normalize(viewPoint - cameraTarget);

    Camera *camera;

    if(input) { 
        glm::vec3 viewPoint = glm::vec3(0,2,0);
        glm::vec3 cameraTarget = glm::vec3(.7,.7,3);
        glm::vec3 viewDir = glm::normalize(cameraTarget - viewPoint);
        glm::vec3 upward = glm::vec3(0,1,0);
        float focalLength = 10;

        PerspCamera perspCamera(viewPoint, viewDir, upward, imageWidth, imageHeight, focalLength);

        camera = &perspCamera;
    }
    else {

        glm::vec3 viewPoint = glm::vec3(0,0,0);
        glm::vec3 cameraTarget = glm::vec3(0,0,1);
        glm::vec3 viewDir = glm::normalize(cameraTarget - viewPoint);
        glm::vec3 upward = glm::vec3(0,1,0);

        OrthoCamera orthoCamera(viewPoint, viewDir, upward, imageWidth, imageHeight);

        camera = &orthoCamera;
    }
    

    float I = 1; // Between 0-1
    glm::vec3 lightDirection(0,10,0);
    DirectionalLight dLight(I, lightDirection);

    glm::vec3 planeOrigin(0,0,0);
    glm::vec3 planeColor(128,128,128);
    Material planeMaterial(planeColor, 0.3f, 0.3f, 0.4f, true);
    glm::vec3 planeNormal(0,1,0);
    Plane plane(planeOrigin, planeMaterial, planeNormal);

    glm::vec3 sphereOrigin1(2,2,10);
    float radius1 = 2;
    glm::vec3 color1(255,0,0);
    Material sphereMaterial(color1, 0.2f , 0.4f, 0.4f, false);
    Sphere sphere1(sphereOrigin1, sphereMaterial, radius1);

    // glm::vec3 sphereOrigin2(0,-100,0);
    // float radius2 = 250;
    // glm::vec3 color2(0,255,0);
    // Material sphere2Mat(color2, 0);
    // Sphere sphere2(sphereOrigin2, sphere2Mat, radius2);

    // Tetrahedron
    glm::vec3 origin(0,0,0);
    glm::vec3 tColor1(0,128,128);
    glm::vec3 tColor2(0,128,128);
    Material triangleMat1(tColor1, 0.4f, 0.0f, 0.6f, false);
    Material triangleMat2(tColor2, 0.4f, 0.3f, 0.3f, false);

    // glm::vec3 vertexA = glm::vec3(9,0,6) * 50.0f;
    // glm::vec3 vertexB = glm::vec3(6,0,0) * 50.0f;
    // glm::vec3 vertexC = glm::vec3(4,0,3) * 50.0f;
    // glm::vec3 vertexD = glm::vec3(7,5,2) * 50.0f;

    // Triangle frontTriangle(origin, triangleMat2, vertexA, vertexB, vertexD);
    // Triangle bottomTriangle(origin, triangleMat1 , vertexA, vertexC, vertexB);
    // Triangle leftTriangle(origin, triangleMat1, vertexD, vertexC, vertexA);
    // Triangle rightTriangle(origin, triangleMat1, vertexC, vertexD, vertexB);

    glm::vec3 vertex1 = glm::vec3(4,0,3);
    glm::vec3 vertex2 = glm::vec3(2,0,5);
    glm::vec3 vertex3 = glm::vec3(2,0,3);
    glm::vec3 vertex4 = glm::vec3(3,1,4);

    Triangle backTriangle2(origin, triangleMat1, vertex4, vertex1, vertex2);
    Triangle bottomTriangle2(origin, triangleMat1 , vertex1, vertex2, vertex3);
    Triangle leftTriangle2(origin, triangleMat2, vertex1, vertex3, vertex4);
    Triangle rightTriangle2(origin, triangleMat1, vertex2, vertex3, vertex4);


    // Surface* objects[objectCount] = {&sphere1};
    Surface* objects[objectCount] = {&sphere1, &backTriangle2, &bottomTriangle2, &leftTriangle2, 
                                    &rightTriangle2, &plane};
    


    // Ray Equation: p(t) = e + t(s − e).
    renderImage(camera, image, imageWidth, imageHeight, objects, dLight);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);
        // cameraSwap(window, camera, orthoCamera, perspCamera);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // bind Texture
        glBindTexture(GL_TEXTURE_2D, texture);

        // render container
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();    
    return 0;
}

void renderImage(Camera* camera, unsigned char* image, int imageWidth, int imageHeight, Surface* (&objects)[objectCount], 
                DirectionalLight light) {

    for(int i = 0; i < imageWidth; i++)
    {
        for (int j = 0; j < imageHeight; j++)
        {
            int recursionDepth = 0;
            // compute (u,v) to get coordinates of pixel's position on the image plane, with respect to e
            float u = camera->DeterminePixelU(j, imageWidth); // FLIPPED I AND J AND NOW WORKS????
            float v = camera->DeterminePixelV(i, imageHeight);

            // create ray from pixel position
            glm::vec3 o = camera->GenerateRayOrigin(u,v); 
            glm::vec3 d = camera->GenerateRayDirection(u,v);

            Ray ray(o,d);
            
            float tMin = INFINITY;
            HitRecord hitRecord;
            glm::vec3 color = shade(ray, light, hitRecord, tMin, objects, objectCount, recursionDepth);


            // SHADOWS ! 
            // source: https://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/shadow.pdf

            // glm::vec3 shadowRayDir = light.direction;
            // float epsilon = 0.5f;
            // glm::vec3 shadowRayOrigin = ray.evaluate(tMin) + epsilon*(shadowRayDir);
            
            // Ray shadowRay(shadowRayOrigin, shadowRayDir);
            // bool inShadow = shadowRay.inShadow(objects, objectCount);
            // if (inShadow)
            //     color = glm::vec3(0,0,0);
                
            
            if (tMin != INFINITY) { // if t is not infinity (hit)
                int idx = (i * imageWidth + j) * 3;
                image[idx] = color.x;
                image[idx+1] = color.y;
                image[idx+2] = color.z;
            }
            else {
                int idx = (i * imageWidth + j) * 3;
                image[idx] = 0;
                image[idx+1] = 0;
                image[idx+2] = 0;
            }
        }
    }

    unsigned char *data = &image[0];
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
}

glm::vec3 shade(Ray &ray, DirectionalLight &light, HitRecord& hitRecord, float &tMin, Surface** objects, int objectCount, int &recursionDepth) {
            glm::vec3 color = glm::vec3(0,0,0);
            for (int i = 0; i < objectCount; i++) {
                hitRecord = objects[i]->hit(ray, 0, INFINITY);
                if (hitRecord.t < tMin) {
                     tMin = hitRecord.t;
                     color = light.illuminate(ray, hitRecord);
                }
            }


            // Mirror Reflection (Glaze)
            // source: https://web.cse.ohio-state.edu/~shen.94/681/Site/Slides_files/reflection_refraction.pdf
            if (hitRecord.t != INFINITY) {
                if (recursionDepth < maxRecursionDepth) {
                    if (hitRecord.s->material.glazed) {
                        glm::vec3 reflectionDir = -2 * (glm::dot(ray.getDirection(), hitRecord.n)) * hitRecord.n + ray.getDirection();
                        float epsilon = 0.5f;
                        glm::vec3 reflectOrigin = ray.evaluate(tMin) + epsilon*(reflectionDir);
                        Ray reflectRay = Ray(reflectOrigin, reflectionDir);
                        recursionDepth += 1;
                        color += k_s * shade(reflectRay, light, hitRecord, tMin, objects, objectCount, recursionDepth);
                    }
                }
            }
            
            // std::cout << "color: " << glm::to_string(color) << std::endl;
            return color;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
}

void cameraSwap(GLFWwindow *window, Camera* camera, OrthoCamera o, PerspCamera p) {
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {

        if (SWAP) {
            camera = &o;
            SWAP = !SWAP;
        }
        else {
            camera = &p;
            SWAP = !SWAP;
        }
        std::cout << "SWAP: " << SWAP << std::endl;
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// Camera
//   viewpoint, camera,basis
//   resolution
//   depth
//   left, right, top

//   for i -> 128
//       for j -> 128
//           Ray = Camera(i,j)
//           for obj in Objects in scene
//               intersection_point = obj.intersect(Ray)
//               color = shading(int-point, normal)
//           image[i][j] = color