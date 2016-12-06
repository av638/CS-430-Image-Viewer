#define GLFW_DLL 1

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>
#include "linmath.h"
#include <assert.h>


// Create the structure for the vertex
typedef struct
{
  float Position[2];
  float TexCoord[2];
} Vertex;

// Create the structure for the image NOTE "don't care about the alpha channel"
typedef struct Pixmap
{
    int width, height, magicNumber;
    unsigned char *image;
} Pixmap;


// Create all the vertexes to be used for properly displaying the image
Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0.99999}},
  {{1, 1},  {0.99999, 0}},
  {{-1, 1}, {0, 0}},
  {{-1, 1}, {0, 0}},
  {{-1, -1}, {0, 0.99999}},
  {{1, -1}, {0.99999, 0.99999}}
};



const double pi = 3.1415926535897;
float rotation = 0;
float scale = 1;
float translate_x = 0;
float translate_y = 0;
float shear_x = 0;
float shear_y = 0;


// Same vertex shader from the texDemo
static const char* vertex_shader_text =
    "uniform mat4 MVP;\n"
    "attribute vec2 TexCoordIn;\n"
    "attribute vec2 vPos;\n"
    "varying vec2 TexCoordOut;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    TexCoordOut = TexCoordIn;\n"
    "}\n";

// Same fragment shader from the texDemo
static const char* fragment_shader_text =
    "varying lowp vec2 TexCoordOut;\n"
    "uniform sampler2D Texture;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
    "}\n";

// Prints out an appropriate error
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}


// This function will perform all of the affine transformations on the loaded image
// Whenever a key is pressed we will change/affect the loaded image
//
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Hit escape to quite the ez-view program
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Rotate the image Left wise 90 degrees using W key
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    	rotation += 90*pi/180;

    // Rotate the image Right wise 90 degrees using E key
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    	rotation -= 90*pi/180;

    // Zoom into the image using =	key
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)//

    	scale *= 2;

    // Zoom out of the image using - key
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
    	scale *= .5;

    // Translate the image down using down arrow
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    	translate_y += .1;

    // Translate the image Up using up arrow
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    	translate_y -= .1;

    // Translate the image left using left arrow
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    	translate_x += .1;

    // Translate the image right using right arrow
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    	translate_x -= .1;

    // Shear image upward using W key
    if (key == GLFW_KEY_W && action == GLFW_PRESS) //Shear Up
    	shear_y += .1;

    // Shear image downward using S key
    if (key == GLFW_KEY_S && action == GLFW_PRESS) //Shear Down
    	shear_y -= .1;

    // Shear image right using D key
    if (key == GLFW_KEY_D && action == GLFW_PRESS) //Shear Right
    	shear_x += .1;

    // Shear image left using A key
    if (key == GLFW_KEY_A && action == GLFW_PRESS) //Shear Left
    	shear_x -= .1;
}

// Same Compile shade checker from the tex demo
// used to compile the shader as well as check if it has compiled properly
// if not it exits the program as there is no reason to continue
void glCompileShaderOrDie(GLuint shader)
{
    GLint compiled;
    glCompileShader(shader);
    glGetShaderiv(shader,
        GL_COMPILE_STATUS,
        &compiled);

    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader,
              GL_INFO_LOG_LENGTH,
              &infoLen);
        char* info = malloc(infoLen+1);
        GLint done;
        glGetShaderInfoLog(shader, infoLen, &done, info);
        printf("Unable to compile shader: %s\n", info);
        exit(-1);
    }
}


// Main will both load the ppm image be it P6 or P3
// and will load that image into the ez-view application in order to
// perform some affine transformations on it
int main(int argc, char *argv[])
{
    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location;

    // Create variables for the image loading
     FILE *source;
    int magicNumber;
    char c;
    int width, height, maxColor;
    int i, j, size, totalItemsRead, pixel;

    //Create a buffer for the pixmap image
    Pixmap *buffer = (Pixmap *)malloc(sizeof(Pixmap));

    source = fopen (argv[1], "r");
    if(source == NULL)
    {
        fprintf(stderr, "\nERROR: File cannot be opened & or does not Exist!");
        fclose(source);
        free(buffer);
        exit(-1);
    }

    fscanf(source, "P%c\n", &c);
    magicNumber = c -'0';// convert the magic number over to an int
    //printf("%d", magicNumber);
    //printf("%c", c);
    if (magicNumber != 6 && magicNumber != 3 ) //if not in either p6 or p3 format then exit
    {
        fprintf(stderr, "\nERROR: This is not in the correct ppm format!");
        free(buffer);
        fclose(source);
        exit(-1);
    }

    c = getc(source);
    //skip the comments since they do not matter
    while(c =='#')
    {
        c = getc(source);
        while(c!='\n') //read to the end of the line
        {
            c = getc(source);
        }
    }
    // get it ready to read in the width, height, max color
    ungetc(c, source);

    // read in the width, height. and max color value
    fscanf(source, "%d %d %d\n", &width, &height, &maxColor);
    //printf("%d", width);
    //printf("%d", maxColor);
    if(maxColor > 255 || maxColor <= 0){
        fprintf(stderr,"\nERROR: Image is not 8 bits per channel!");
        fclose(source);
        free(buffer);
        exit(-1);
    }
    // mult the size by three to account for rgb
    size = width * height * 3;

    // Put everything necessary into the buffer to be used to write it out into a different or even same format.
    // Except for the data because we just want to make sure that we allocate enough memory first.
    if(!buffer)
    {
        fprintf(stderr, "\nERROR: Cannot allocate memory for the ppm image.");
        fclose(source);
        free(buffer);
        exit(-1);
    }
    else{
        buffer->width = width;
        buffer->height = height;
        buffer->magicNumber = magicNumber;
        // Allocate memory for the entire image and mult by three to account for RGB
        buffer->image = (unsigned char *)malloc(width*height*3);
    }

    if(!buffer->image){
        fprintf(stderr,"\nERROR: Cannot allocate memory for the ppm image!");
        fclose(source);
        free(buffer);
        exit(-1);
    }
    //printf("\nBuffer successfully created with all stuff!\n");
    // Read the image into the buffer depending on whether it is in P6 or P3 format
    // If its raw bits
    if(magicNumber == 6)
    {   // Read from the file the entire size of the image at a One Byte size into the buffer
        totalItemsRead = fread((void *) buffer->image, 1, (size_t) size, source);
        if (totalItemsRead != size)
        {
            fprintf(stderr,"\nERROR: Could not read the entire image! \n");
            fclose(source);
            free(buffer);
            exit(-1);
        }
    }
    else if(magicNumber == 3)
    {
        for(i=0;i<height;i++)
        {
            for(j=0;j<width;j++)
            {
                fscanf(source, "%d ", &pixel);
                buffer->image[i*width*3+3*j] = pixel;
                fscanf(source, "%d ", &pixel);
                buffer->image[i*width*3+3*j+1] = pixel;
                fscanf(source, "%d ", &pixel);
                buffer->image[i*width*3+3*j+2] = pixel;
            }
        }
    }

    fclose(source);

    if (!glfwInit())
        exit(EXIT_FAILURE);


    // Same hints used in the demo provided
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "EZ-View", NULL, NULL);

    // If the window does not initiate then terminate the program and close the window
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Turn on key callback in order to take in the user inputs
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);



    // NOTE: OpenGL error checks have been omitted for brevity
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);



    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          (void*) (sizeof(float) * 2));

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 width,
                 height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 buffer->image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);


    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int windowWidth, windowHeight;
        mat4x4 r, h, s, t, rh, rhs, mvp; //matrices for each transformation and intermediate values

        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        ratio = windowWidth / (float) windowHeight;

        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        //RHS T
        mat4x4_identity(r);
        mat4x4_rotate_Z(r, r, rotation); //angle of rotation

        mat4x4_identity(h);
        h[0][1] = shear_x;
        h[1][0] = shear_y;


        mat4x4_identity(s); //NOT WORKING AS INTENDED
        s[0][0] = s[0][0]*scale;
        s[1][1] = s[1][1]*scale;

        mat4x4_identity(t);
        mat4x4_translate(t, translate_x, translate_y, 0);

        mat4x4_mul(rh, r, h); //R*H
        mat4x4_mul(rhs, rh, s);//R*H*S
        mat4x4_mul(mvp, rhs, t);//R*H*S*T

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    free(buffer);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
