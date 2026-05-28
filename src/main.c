#include "img.h"
#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

unsigned int window_w = 800;
unsigned int window_h = 600;

float img_scale = 1.0f;
float img_offset_x = 0.0f;
float img_offset_y = 0.0f;

float vertices[] = {
    /* Position          TexCoord */
     1.0f,  1.0f, 0.0f,  1.0f, 1.0f, /* top right */
     1.0f, -1.0f, 0.0f,  1.0f, 0.0f, /* bottom right */
    -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, /* bottom left */
    -1.0f,  1.0f, 0.0f,  0.0f, 1.0f  /* top left */
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};
/*
 * glfw window resize callback, whenever the window size changed (by OS or user
 * resize) this callback function excutes.
 */
void window_resize_callback(GLFWwindow *window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

void check_compile_errors(GLuint shader, const char *type);

int main(int argc, char *argv[])
{
    GLFWwindow *window;

    /* Initialize glfw */
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create glfw window */
    window = glfwCreateWindow(window_w, window_h, "Image Viewer", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwSetFramebufferSizeCallback(window, window_resize_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    /* Make OpenGL context */
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    GLuint vs, fs;
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_code, NULL);
    glCompileShader(vs);
    check_compile_errors(vs, "VERTEX");

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_code, NULL);
    glCompileShader(fs);
    check_compile_errors(fs, "FRAGMENT");

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);
    check_compile_errors(shader, "PROGRAM");

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    /*
     * set the texture wrapping parameters
     * set texture wrapping to GL_REPEAT (default wrapping method)
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    /* set texture filtering parameters */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
    {
        printf("Error: Could not open or find file '%s'\n", argv[1]);
        return -1;
    }

    unsigned int w, h, ch;
    unsigned char *img;
    img = read_image(fp, &w, &h, &ch);
    if (img == NULL) return -1;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
    glGenerateMipmap(GL_TEXTURE_2D);

    fclose(fp);
    free(img);

    GLint tex_trs = glGetUniformLocation(shader, "tex_trs");

    while (!glfwWindowShouldClose(window))
    {
        float r_img = (float)w / (float)h;
        float r_win = (float)window_w / (float)window_h;

        float base_scale_x, base_scale_y;
        if (r_img > r_win)
        {
            base_scale_x = 1.0f;
            base_scale_y = r_img / r_win;
        }
        else
        {
            base_scale_x = r_win / r_img;
            base_scale_y = 1.0f;
        }

        float scale_x = base_scale_x / img_scale;
        float scale_y = base_scale_y / img_scale;

        float center_x = 0.5f;
        float center_y = 0.5f;

        float final_offset_x = center_x - center_x * scale_x - img_offset_x;
        float final_offset_y = center_y - center_y * scale_y - img_offset_y;

        float tex_mat[16] = {
            scale_x,        0.0f,           0.0f,   0.0f,
            0.0f,           scale_y,        0.0f,   0.0f,
            0.0f,           0.0f,           1.0f,   0.0f,
            final_offset_x, final_offset_y, 0.0f,   1.0f
        };

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, texture);
        glUseProgram(shader);
        glUniformMatrix4fv(tex_trs, 1, GL_FALSE, tex_mat);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();
    return 0;
}

void window_resize_callback(GLFWwindow *window, int width, int height)
{
    window_w = width;
    window_h = height;
    glViewport(0, 0, width, height);
}
int is_dragging = 0;
double last_mouse_x = 0.0;
double last_mouse_y = 0.0;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    float zoom_factor = 0.05f;
    if (yoffset > 0) img_scale += zoom_factor * img_scale;
    else if (yoffset < 0) img_scale -= zoom_factor * img_scale;

    if (img_scale < 0.1f)  img_scale = 0.1f;
    if (img_scale > 20.0f) img_scale = 20.0f;
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            is_dragging = 1;
            /* Get initial position when click happens */
            glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);
        }
        else if (action == GLFW_RELEASE)
        {
            is_dragging = 0;
        }
    }
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (is_dragging)
    {
        /* Calculate delta movement in pixels */
        double delta_x =  xpos - last_mouse_x;
        double delta_y = -ypos + last_mouse_y;

        /* Convert pixel movement to UV coordinate space changes 
         * Multiply by img_scale so panning speed matches zoom depth */
        img_offset_x += (float)(delta_x / window_w) / img_scale;
        img_offset_y -= (float)(delta_y / window_h) / img_scale; 
        /* Note: img_offset_y uses -= because window coords go top-to-bottom,
         * while OpenGL texture/UV coordinates usually go bottom-to-top */
    }

    /* Always update old positions to current positions */
    last_mouse_x = xpos;
    last_mouse_y = ypos;
}

void check_compile_errors(GLuint shader, const char *type)
{
    int success;
    char info[1024];
    if (strcmp(type, "PROGRAM") == 0)
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, info);
            printf("ERROR::PROGRAM_LINKING_ERROR of type: %s\n%s\n", type, info);
        }
    }
    else
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, info);
            printf("ERROR::SHADER_COMPILATION_ERROR of type: %s\n%s\n", type, info);
        }

    }
}
