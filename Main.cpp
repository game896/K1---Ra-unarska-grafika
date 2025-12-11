#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <cmath> 
#include <algorithm>
#include <iostream>
#include "Util.h"
#include <chrono>
#include <thread>
#include <sstream>   
#include <vector>


#define NUM_SLICES 40
#define TARGET_FPS 75

float doorY = 0.7f;       
float doorTargetY = 0.7f; 
float doorSpeed = 0.1f;   
bool acActive = false;
float rectScaleY = 0.0f;
float waterFillSpeed = 0.0005f;
float airTemp = 30.0f;
float desiredAirTemp = 24.0f;
float uX = 0.0f;
float uY = -0.5f;

GLFWcursor* cursor;
GLFWcursor* cursorPressed;

unsigned int lowDesiredNum = -1;

int screenWidth = 1;
int screenHeight = 1;

unsigned int numberTextures[18]; 

unsigned int getNumberTexture(int number) {
    if (number < 0) number = 0;
    if (number > 9) number = 9;
    return numberTextures[number];
}

int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}

float initialJumpTime = -1.0f;

void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath);
    glBindTexture(GL_TEXTURE_2D, texture);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void loadNumberTextures() {
    preprocessTexture(numberTextures[0], "res/0.png");
    preprocessTexture(numberTextures[1], "res/1.png");
    preprocessTexture(numberTextures[2], "res/2.png");
    preprocessTexture(numberTextures[3], "res/3.png");
    preprocessTexture(numberTextures[4], "res/4.png");
    preprocessTexture(numberTextures[5], "res/5.png");
    preprocessTexture(numberTextures[6], "res/6.png");
    preprocessTexture(numberTextures[7], "res/7.png");
    preprocessTexture(numberTextures[8], "res/8.png");
    preprocessTexture(numberTextures[9], "res/9.png");
    preprocessTexture(numberTextures[10], "res/minus.png");
    preprocessTexture(numberTextures[11], "res/fire.png");
    preprocessTexture(numberTextures[12], "res/snow.png");
    preprocessTexture(numberTextures[13], "res/ok.png");
    preprocessTexture(numberTextures[14], "res/name.png");
    preprocessTexture(numberTextures[15], "res/desired.png");
    preprocessTexture(numberTextures[16], "res/actual.png");
    preprocessTexture(numberTextures[17], "res/heat.png");
}

void formVAOs(float* verticesRect, size_t rectSize, unsigned int& VAOrect) {

    unsigned int VBOrect;
    glGenVertexArrays(1, &VAOrect);
    glGenBuffers(1, &VBOrect);

    glBindVertexArray(VAOrect);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrect);
    glBufferData(GL_ARRAY_BUFFER, rectSize, verticesRect, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

}

void formTexturedVAOs(float* vertices, size_t size, unsigned int& VAO) {
    unsigned int VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void generateCircleVertices(std::vector<float>& verts, float cx, float cy, float r, float rCol, float gCol, float bCol, float aCol)
{
    const int N = 50; 
    verts.clear();
    verts.reserve((N + 2) * 6);

    verts.push_back(cx);
    verts.push_back(cy);
    verts.push_back(rCol);
    verts.push_back(gCol);
    verts.push_back(bCol);
    verts.push_back(aCol);

    float aspect = (float)screenHeight / (float)screenWidth;

    for (int i = 0; i <= N; i++)
    {
        float angle = 2.0f * M_PI * i / N;
        float x = cx + cos(angle) * r * aspect;
        float y = cy + sin(angle) * r;
        verts.push_back(x);
        verts.push_back(y);
        verts.push_back(rCol);
        verts.push_back(gCol);
        verts.push_back(bCol);
        verts.push_back(aCol);
    }

}

void drawLight(unsigned int rectShader, unsigned int VAOrect) {
    glUseProgram(rectShader);

    glUniform1f(glGetUniformLocation(rectShader, "uX"), 0.0f);
    glUniform1f(glGetUniformLocation(rectShader, "uY"), -0.5f);
    glUniform1f(glGetUniformLocation(rectShader, "uScaleY"), 1.0f);

    float r = acActive ? 0.0f : 1.0f;
    float g = acActive ? 1.0f : 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    int loc = glGetUniformLocation(rectShader, "overrideColor");
    glUniform4f(loc, r, g, b, a);

    glBindVertexArray(VAOrect);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 52);

    glUniform4f(loc, -1.0f, 0.0f, 0.0f, 0.0f);
}

void drawGrowing(unsigned int rectShader, unsigned int VAOrectGrowing, float scaleY) {
    glUseProgram(rectShader);

    glUniform1f(glGetUniformLocation(rectShader, "uX"), 0.0f);
    glUniform1f(glGetUniformLocation(rectShader, "uY"), -0.7f);

    glUniform1f(glGetUniformLocation(rectShader, "uScaleY"), scaleY);

    glBindVertexArray(VAOrectGrowing);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawDoor(unsigned int rectShader, unsigned int VAOrectDoor, float deltaTime) {
    if (acActive) doorTargetY = 0.6f;   
    else doorTargetY = 0.7f;            

    float diff = doorTargetY - doorY;
    float maxStep = doorSpeed * deltaTime;
    if (fabs(diff) <= maxStep) doorY = doorTargetY;
    else doorY += (diff > 0 ? maxStep : -maxStep);

    glUseProgram(rectShader);
    glUniform1f(glGetUniformLocation(rectShader, "uX"), 0.0f); 
    glUniform1f(glGetUniformLocation(rectShader, "uY"), doorY);
    glUniform1f(glGetUniformLocation(rectShader, "uScaleY"), 1.0f);

    glBindVertexArray(VAOrectDoor);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void drawTexturedRect(unsigned int shader, unsigned int VAO, unsigned int texture) {
    glUseProgram(shader);
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    glUniform1f(glGetUniformLocation(shader, "uX"), 0.0f);
    glUniform1f(glGetUniformLocation(shader, "uY"), 0.0f);
    glUniform1f(glGetUniformLocation(shader, "uScaleY"), 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

}

void drawRect(unsigned int rectShader, unsigned int VAOrect) {
    glUseProgram(rectShader); 
    glUniform1f(glGetUniformLocation(rectShader, "uX"), uX); 
    glUniform1f(glGetUniformLocation(rectShader, "uY"), uY);
    glUniform1f(glGetUniformLocation(rectShader, "uScaleY"), 1.0f);

    glBindVertexArray(VAOrect); 
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void squish_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        rectScaleY = 0.0f;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_UP && action == GLFW_PRESS && acActive == true)
    {
        if((int)desiredAirTemp < 40)
            ++desiredAirTemp;
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS && acActive == true)
    {
        if ((int)desiredAirTemp > -10)
            --desiredAirTemp;
    }
}

void center_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwSetCursor(window, cursorPressed);


        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float x = (float)xpos / screenWidth * 2.0f - 1.0f;
        float y = 1.0f - (float)ypos / screenHeight * 2.0f;

        if (x > 0.64f && y > 0.44f && x < 0.67f && y < 0.47f) {
            acActive = !acActive;
            return;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        glfwSetCursor(window, cursor);
    }

}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Vezba 3", monitor, NULL);

    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, squish_callback);
    glfwSetMouseButtonCallback(window, center_callback);

    cursor = loadImageToCursor("res/acRemote.png");
    cursorPressed = loadImageToCursor("res/acRemotePressed.png");
    glfwSetCursor(window, cursor);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int rectShader = createShader("rect.vert", "rect.frag");
    unsigned int textureShader = createShader("texture.vert", "texture.frag");

    loadNumberTextures();

    float verticesRect[] = {
         -0.7f, 1.45f, 1.0f, 1.0f, 1.0f, 1.0f, // gornje levo teme
         -0.7f, 0.905f, 1.0f, 1.0f, 1.0f, 1.0f,// donje levo teme
         0.7f, 0.905f, 1.0f, 1.0f, 1.0f, 1.0f,// donje desno teme
         0.7f, 1.45f, 1.0f, 1.0f, 1.0f, 1.0f,// gornje desno teme
    };

    float verticesRectBlack[] = {
         -0.295f, 1.06f, 0.0f, 0.0f, 0.0f, 1.0f,
         -0.295f, 0.905f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.295f, 0.905f, 0.0f, 0.0f, 0.0f, 1.0f,
         0.295f, 1.06f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDoor[] = {
         -0.29f, -0.15f, 1.0f, 1.0f, 1.0f, 1.0f,
         -0.29f, -0.29f, 1.0f, 1.0f, 1.0f, 1.0f,
         0.29f, -0.29f, 1.0f, 1.0f, 1.0f, 1.0f,
         0.29f, -0.15f, 1.0f, 1.0f, 1.0f, 1.0f,
    };

    float verticesRectGrowing[] = {
         -0.6f, 0.0f, 0.5f, 0.8f, 1.0f, 1.0f,
         -0.6f, 0.5f, 0.5f, 0.8f, 1.0f, 1.0f,
         -0.4f, 0.5f, 0.5f, 0.8f, 1.0f, 1.0f,   
         -0.4f, 0.0f, 0.5f, 0.8f, 1.0f, 1.0f,  
    };


    float verticesRectBucket[] = {
        -0.61f, -0.22f, 1.0f, 1.0f, 1.0f, 0.3f,
        -0.61f, 0.32f, 1.0f, 1.0f, 1.0f, 0.3f,
        -0.39f, 0.32f, 1.0f, 1.0f, 1.0f, 0.3f,
        -0.39f, -0.22f, 1.0f, 1.0f, 1.0f, 0.3f,
    };

    std::vector<float> verticesCircleLight;
    generateCircleVertices(
        verticesCircleLight,
        0.655f, 0.96f,     
        0.02f,             
        1.0f, 1.0f, 1.0f, 0.3f 
    );


    float verticesRectDesiredLowerBackground[] = {
        0.38f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.38f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.45f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.45f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDispDesiredLower[] = {
        0.38f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        0.38f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        0.45f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        0.45f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };

    float verticesRectDesiredHigherBackground[] = {
        0.31f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.31f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.38f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.38f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDispDesiredHigher[] = {
        0.31f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        0.31f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        0.38f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        0.38f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };

    float verticesRectDesiredMinusBackground[] = {
        0.24f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.24f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.31f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.31f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDispDesiredMinus[] = {
        0.24f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        0.24f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        0.31f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        0.31f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };
    float verticesRectActualLowerBackground[] = {
        0.03f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.03f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.1f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.1f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDispActualLower[] = {
        0.03f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        0.03f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        0.1f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        0.1f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };
    float verticesRectActualHigherBackground[] = {
        -0.04f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.04f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.03f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.03f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDispActualHigher[] = {
        -0.04f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        -0.04f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        0.03f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        0.03f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };

    float verticesRectActualMinusBackground[] = {
        -0.11f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.11f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.04f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.04f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectDispActualMinus[] = {
        -0.11f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        -0.11f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        -0.04f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        -0.04f, 0.7f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };
    float verticesRectImageBackground[] = {
        -0.33f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.33f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.24f, 1.33f, 0.0f, 0.0f, 0.0f, 1.0f,
        -0.24f, 1.19f, 0.0f, 0.0f, 0.0f, 1.0f,
    };

    float verticesRectImage[] = {
        -0.33f, 0.69f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 0.0f,
        -0.33f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 0.0f, 1.0f,
        -0.24f, 0.83f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 1.0f,
        -0.24f, 0.69f, 0.675f, 0.82f, 0.416f, 1.0f, 1.0f, 0.0f
    };

    float verticesRectPipe[] = {
        -0.51f, 0.30f, 1.0f, 1.0f, 1.0f, 1.0f,
        -0.51f, 0.91f, 1.0f, 1.0f, 1.0f, 1.0f,
        -0.50f, 0.91f, 1.0f, 1.0f, 1.0f, 1.0f,
        -0.50f, 0.30f, 1.0f, 1.0f, 1.0f, 1.0f,
    };

    float verticesNameImage[] = {
         0.66f, -0.96f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 0.0f,  
         0.66f, -0.84f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 1.0f,   
         0.98f, -0.84f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 1.0f,   
         0.98f, -0.96f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 0.0f    
    };

    float verticesDesiImage[] = {
         0.26f,  0.83f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 0.0f,
         0.26f,  0.93f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 1.0f,
         0.45f,  0.93f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 1.0f,
         0.45f,  0.83f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 0.0f
    };

    float verticesActImage[] = {
         -0.09f,  0.83f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 0.0f,
         -0.09f,  0.93f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 1.0f,
         0.12f,  0.93f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 1.0f,
         0.12f,  0.83f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 0.0f
    };

    float verticesHeatImage[] = {
         -0.34f,  0.83f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 0.0f,
         -0.34f,  0.93f,   1.0f, 0.82f, 0.416f, 1.0f,   0.0f, 1.0f,
         -0.17f,  0.93f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 1.0f,
         -0.17f,  0.83f,   1.0f, 0.82f, 0.416f, 1.0f,   1.0f, 0.0f
    };

    unsigned int VAOrect;
    unsigned int VAOrectBlack;
    unsigned int VAOrectDoor;
    unsigned int VAOrectGrowing;
    unsigned int VAOrectBucket;
    unsigned int VAOrectPipe;
    unsigned int VAORectDispDesiredLower;
    unsigned int VAORectDesiredLowerBackground;
    unsigned int VAORectDesiredHigherBackground;
    unsigned int VAORectDispDesiredHigher;
    unsigned int VAORectDesiredMinusBackground;
    unsigned int VAORectDispDesiredMinus;
    unsigned int VAORectActualLowerBackground;
    unsigned int VAORectDispActualLower;
    unsigned int VAORectActualHigherBackground;
    unsigned int VAORectDispActualHigher;
    unsigned int VAORectActualMinusBackground;
    unsigned int VAORectDispActualMinus;
    unsigned int VAORectNameImage;
    unsigned int VAORectImageBackground;
    unsigned int VAORectImage;
    unsigned int VAOlight;
    unsigned int VAODesiImage;
    unsigned int VAOActImage;
    unsigned int VAOHeatImage;

    formVAOs(verticesRect, sizeof(verticesRect), VAOrect);
    formVAOs(verticesRectBlack, sizeof(verticesRectBlack), VAOrectBlack);
    formVAOs(verticesRectDoor, sizeof(verticesRectDoor), VAOrectDoor);
    formVAOs(verticesRectGrowing, sizeof(verticesRectGrowing), VAOrectGrowing);
    formVAOs(verticesRectBucket, sizeof(verticesRectBucket), VAOrectBucket);
    formVAOs(verticesCircleLight.data(), verticesCircleLight.size() * sizeof(float), VAOlight);
    formVAOs(verticesRectPipe, sizeof(verticesRectPipe), VAOrectPipe);
    formVAOs(verticesRectDesiredLowerBackground, sizeof(verticesRectDesiredLowerBackground), VAORectDesiredLowerBackground);
    formVAOs(verticesRectDesiredHigherBackground, sizeof(verticesRectDesiredHigherBackground), VAORectDesiredHigherBackground);
    formVAOs(verticesRectDesiredMinusBackground, sizeof(verticesRectDesiredMinusBackground), VAORectDesiredMinusBackground);
    formVAOs(verticesRectActualLowerBackground, sizeof(verticesRectActualLowerBackground), VAORectActualLowerBackground);
    formVAOs(verticesRectActualHigherBackground, sizeof(verticesRectActualHigherBackground), VAORectActualHigherBackground);
    formVAOs(verticesRectActualMinusBackground, sizeof(verticesRectActualMinusBackground), VAORectActualMinusBackground);
    formVAOs(verticesRectImageBackground, sizeof(verticesRectImageBackground), VAORectImageBackground);
    formTexturedVAOs(verticesRectDispDesiredLower, sizeof(verticesRectDispDesiredLower), VAORectDispDesiredLower);
    formTexturedVAOs(verticesRectDispDesiredHigher, sizeof(verticesRectDispDesiredHigher), VAORectDispDesiredHigher);
    formTexturedVAOs(verticesRectDispDesiredMinus, sizeof(verticesRectDispDesiredMinus), VAORectDispDesiredMinus);
    formTexturedVAOs(verticesRectDispActualLower, sizeof(verticesRectDispActualLower), VAORectDispActualLower);
    formTexturedVAOs(verticesRectDispActualHigher, sizeof(verticesRectDispActualHigher), VAORectDispActualHigher);
    formTexturedVAOs(verticesRectDispActualMinus, sizeof(verticesRectDispActualMinus), VAORectDispActualMinus);
    formTexturedVAOs(verticesRectImage, sizeof(verticesRectImage), VAORectImage);
    formTexturedVAOs(verticesNameImage, sizeof(verticesNameImage), VAORectNameImage);
    formTexturedVAOs(verticesDesiImage, sizeof(verticesDesiImage), VAODesiImage);
    formTexturedVAOs(verticesActImage, sizeof(verticesActImage), VAOActImage);
    formTexturedVAOs(verticesHeatImage, sizeof(verticesHeatImage), VAOHeatImage);

    glClearColor(0.05f, 0.15f, 0.05f, 1.0f);

    int frameCount = 0;
    float lastTime = glfwGetTime();

    double frameTime = 1.0 / TARGET_FPS;
    double lastFrame = glfwGetTime();
    float epsilon = 0.01f;
    unsigned int imageTexture = numberTextures[13];

    while (!glfwWindowShouldClose(window)) {
        double startFrame = glfwGetTime();

        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        if (rectScaleY >= 1.0f)  
            acActive = false;


        if (acActive)
        {
            rectScaleY += waterFillSpeed;
            if (fabs(desiredAirTemp - airTemp) < epsilon)
            {
                airTemp = desiredAirTemp;  
                imageTexture = numberTextures[13];
            }
            else if (desiredAirTemp > airTemp)
            {
                airTemp += 0.01f;
                imageTexture = numberTextures[11];
            }
            else if (desiredAirTemp < airTemp)
            {
                airTemp -= 0.01f;
                imageTexture = numberTextures[12];
            }
        }
        else
        {
            if (fabs(30.0f - airTemp) < epsilon)
            {
                airTemp = 30.0f;   
            }
            else if (30.0f > airTemp)
            {
                airTemp += 0.01f;
            }
            else if (30.0f < airTemp)
            {
                airTemp -= 0.01f;
            }
        }

        int onesDesired = (int)abs(desiredAirTemp) % 10;
        int tensDesired = (int)abs(desiredAirTemp) / 10;

        int onesActual = (int)abs(airTemp) % 10;
        int tensActual = (int)abs(airTemp) / 10;

        glClear(GL_COLOR_BUFFER_BIT);

        drawRect(rectShader, VAOrect);
        drawRect(rectShader, VAOrectBlack);
        drawDoor(rectShader, VAOrectDoor, deltaTime);
        drawGrowing(rectShader, VAOrectGrowing, rectScaleY);
        drawRect(rectShader, VAOrectBucket);
        drawLight(rectShader, VAOlight);
        drawRect(rectShader, VAOrectPipe);
        drawRect(rectShader, VAORectDesiredLowerBackground);
        drawRect(rectShader, VAORectDesiredHigherBackground);
        drawRect(rectShader, VAORectDesiredMinusBackground);

        drawRect(rectShader, VAORectActualLowerBackground);
        drawRect(rectShader, VAORectActualHigherBackground);
        drawRect(rectShader, VAORectActualMinusBackground);

        drawRect(rectShader, VAORectImageBackground);
        drawTexturedRect(textureShader, VAORectNameImage, numberTextures[14]);
        drawTexturedRect(textureShader, VAODesiImage, numberTextures[15]);
        drawTexturedRect(textureShader, VAOActImage, numberTextures[16]);
        drawTexturedRect(textureShader, VAOHeatImage, numberTextures[17]);

        if (acActive)
        {
            drawTexturedRect(textureShader, VAORectDispDesiredLower, getNumberTexture(onesDesired));
            drawTexturedRect(textureShader, VAORectDispDesiredHigher, getNumberTexture(tensDesired));
            if ((int)desiredAirTemp < 0)
                drawTexturedRect(textureShader, VAORectDispDesiredMinus, numberTextures[10]);

            drawTexturedRect(textureShader, VAORectDispActualLower, getNumberTexture(onesActual));
            drawTexturedRect(textureShader, VAORectDispActualHigher, getNumberTexture(tensActual));
            if ((int)airTemp < 0)
                drawTexturedRect(textureShader, VAORectDispActualMinus, numberTextures[10]);

            drawTexturedRect(textureShader, VAORectImage, imageTexture);
        }

        // --- FPS counter ---
        frameCount++;
        double now = glfwGetTime();
        if (now - lastTime >= 1.0) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = now;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        // --- precizno čekanje ---
        double endFrame = glfwGetTime();
        double delta = endFrame - startFrame;
        while (delta < frameTime) {
            delta = glfwGetTime() - startFrame;
        }

        lastFrame = startFrame;
    }

    glDeleteProgram(rectShader);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}