/*
 *  Depth of field shader
 *
 *  Key bindings:
 *  p/P        Toggle between perspective/orthogonal/First Person projections
 *
 *
 *  arrows     Change view angle
 *  0          Reset view angle
 *  ESC        Exit
 */

#include "CSCIx229.h"

int mode = 1;       //  Projection type (0-Perspective 1-First Person)
int th = -175;      //  Azimuth of view angle
int ph = 5;         //  Elevation of view angle
double asp = 1;     //  Aspect ratio
int tank = 0;       //  Object
GLuint texture_shader; //  Shader programs

GLuint postproc_shader;
GLint attribute_v_coord, uniform_color_texture, uniform_depth_texture;
GLint uniform_focal_depth, uniform_focal_length, uniform_fstop;
GLint uniform_screen_width, uniform_screen_height;

GLuint fbo, color_texture, depth_texture, rbo_depth;
GLuint vbo_fbo_vertices;

int screen_width = 800;
int screen_height = 600;

float unit = 10.0; // unit length is 10mm

double fp_ex = 45.0;
double fp_ey = 40.0;
double fp_ez = -180.0;
int fp_th = 165; // Azimuth of view angle for first person
int fp_ph = 0; // Elevation of view angle for first person

float filmY = 48;

float focalDepth = 200;  //focal distance value in meters, but you may use autofocus option below
float focalLength = 50; //focal length in mm
float fstop = 2.8; //f-stop value

double fov(double fl)
{
    return 2 * atan(filmY / 2 / fl) * 180 / PI;
}

int init_resources()
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_width, screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glActiveTexture(GL_TEXTURE2);
    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screen_width, screen_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);

    /* Depth buffer */
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width, screen_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    /* Framebuffer to link everything together */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
    GLenum status;
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        return 0;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLfloat fbo_vertices[] = {
            -1, -1,
            1, -1,
            -1, 1,
            1, 1,
    };
    glGenBuffers(1, &vbo_fbo_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_fbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fbo_vertices), fbo_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    return 1;
}

void free_resources()
{
    glDeleteRenderbuffers(1, &rbo_depth);
    glDeleteTextures(1, &color_texture);
    glDeleteFramebuffers(1, &fbo);
    glDeleteBuffers(1, &vbo_fbo_vertices);
    glDeleteProgram(postproc_shader);
}

/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display()
{
    //  Light position and colors
    float Emission[] = {0.0, 0.0, 0.0, 1.0};
    float Ambient[] = {1.0, 1.0, 1.0, 1.0};
    float Diffuse[] = {1.0, 1.0, 1.0, 1.0};
    float Specular[] = {1.0, 1.0, 1.0, 1.0};
    float Position[] = {-50.0f, 100.0f, -50.0f, 1.0};
    float Shinyness[] = {16};

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    //glBindFramebuffer(GL_FRAMEBUFFER, depth_fbo);

    //  Erase the window and the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //  Enable Z-buffering in OpenGL
    glEnable(GL_DEPTH_TEST);
    //  Undo previous transformations
    glLoadIdentity();
    double vx, vy, vz;
    double Ex, Ey, Ez;
    vx = vy = vz = 0.0;

    //  Perspective - set eye position
    if (mode == 0) {
        Ex = -100 * Sin(th) * Cos(ph);
        Ey = +100 * Sin(ph);
        Ez = +100 * Cos(th) * Cos(ph);
        gluLookAt(Ex, Ey, Ez, 0, 0, 0, 0, Cos(ph), 0);
    } else {
        vx = fp_ex - Sin(fp_th);
        vy = fp_ey + Sin(fp_ph);
        vz = fp_ez - Cos(fp_th);
        gluLookAt(fp_ex, fp_ey, fp_ez, vx, vy, vz, 0, 1, 0);
    }

    //  OpenGL should normalize normal vectors
    glEnable(GL_NORMALIZE);
    //  Enable lighting
    glEnable(GL_LIGHTING);
    //  glColor sets ambient and diffuse color materials
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    //  Enable light 0
    glEnable(GL_LIGHT0);
    //  Set ambient, diffuse, specular components and position of light 0
    glLightfv(GL_LIGHT0, GL_AMBIENT, Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, Specular);
    glLightfv(GL_LIGHT0, GL_POSITION, Position);
    //  Set materials
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, Shinyness);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Emission);

    //
    //  Draw scene
    //
    //  Select shader (0 => no shader)
    glUseProgram(texture_shader);

    //  Draw the teapot or cube
    glColor3f(0, 1, 1);
    glPushMatrix();

    glScaled(18, 18, 18);

    glEnable(GL_TEXTURE_2D);
    glCallList(tank);
    glDisable(GL_TEXTURE_2D);

    glPopMatrix();

    /* Post-processing */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(postproc_shader);

    glUniform1i(uniform_color_texture, /*GL_TEXTURE*/1);
    glUniform1i(uniform_depth_texture, /*GL_TEXTURE*/2);
    glUniform1f(uniform_focal_length, focalLength);
    glUniform1f(uniform_focal_depth, focalDepth);
    glUniform1f(uniform_fstop, fstop);
    glUniform1f(uniform_screen_height, screen_height);
    glUniform1f(uniform_screen_width, screen_width);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color_texture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depth_texture);

    glActiveTexture(GL_TEXTURE0);

    glEnableVertexAttribArray(attribute_v_coord);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_fbo_vertices);
    glVertexAttribPointer(
            attribute_v_coord,  // attribute
            2,                  // number of elements per vertex, here (x,y)
            GL_FLOAT,           // the type of each element
            GL_FALSE,           // take our values as-is
            0,                  // no extra data between each position
            0                   // offset of first element
    );
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(attribute_v_coord);

    glUseProgram(0);

    glColor3f(1, 1, 1);

    if (mode == 0) {
        glWindowPos2i(5, 5);
        Print("Angle=%d,%d", th, ph);
    } else {
        glWindowPos2i(5, 45);
        Print("Focal Length=%.1fmm, Focus Distance=%.2fm, f-stop=%.1f", focalLength, focalDepth * unit / 1000.0, fstop);
        glWindowPos2i(5, 25);
        Print("Location (mm): x=%.1f, y=%.1f z=%.1f", fp_ex * unit, fp_ey * unit, fp_ez * unit);
        glWindowPos2i(5, 5);
        Print("Center of View (mm): x=%.1f, y=%.1f z=%.1f", vx * unit, vy * unit, vz * unit);
    }

    //  Render the scene and make it visible
    ErrCheck("display");
    glFlush();
    glutSwapBuffers();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void idle()
{
    //  Tell GLUT it is necessary to redisplay the scene
    glutPostRedisplay();
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key, int x, int y)
{
    if (mode == 0) {
        //  Right arrow key - increase angle by 5 degrees
        if (key == GLUT_KEY_RIGHT)
            th += 5;
            //  Left arrow key - decrease angle by 5 degrees
        else if (key == GLUT_KEY_LEFT)
            th -= 5;
            //  Up arrow key - increase elevation by 5 degrees
        else if (key == GLUT_KEY_UP)
            ph += 5;
            //  Down arrow key - decrease elevation by 5 degrees
        else if (key == GLUT_KEY_DOWN)
            ph -= 5;

        //  Keep angles to +/-360 degrees
        th %= 360;
        ph %= 360;
    } else {
        if (key == GLUT_KEY_UP) {
            fp_ez -= 2 * Cos(fp_th);
            fp_ex -= 2 * Sin(fp_th);
        } else if (key == GLUT_KEY_DOWN) {
            fp_ez += 2 * Cos(fp_th);
            fp_ex += 2 * Sin(fp_th);
        } else if (key == GLUT_KEY_RIGHT) {
            fp_th -= 2;
        } else if (key == GLUT_KEY_LEFT) {
            fp_th += 2;
        } else if (key == GLUT_KEY_PAGE_UP && fp_ph < 90) {
            fp_ph = (fp_ph + 5) % 360;
        } else if (key == GLUT_KEY_PAGE_DOWN && fp_ph > -90) {
            fp_ph = (fp_ph - 5) % 360;
        }
        fp_th %= 360;
    }

    //  Update projection
    Project(fov(focalLength), asp, 1);

    //  Tell GLUT it is necessary to redisplay the scene
    glutPostRedisplay();
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch, int x, int y)
{
    //  Exit on ESC
    if (ch == 27)
        exit(0);
    else if (ch == 'p' || ch == 'P') {
        mode = 1 - mode;
    } else if (ch == '+' || ch == '=') {
        if (focalLength < 200.0) {
            focalLength += 1.0;
        }
    } else if (ch == '-' || ch == '_') {
        if (focalLength > 12.0) {
            focalLength -= 1.0;
        }
    } else if (ch == '[' || ch == '{') {
        if (focalDepth > 5.5) {
            focalDepth -= 5;
        }
    } else if (ch == ']' || ch == '}') {
        if (focalDepth < 1000) {
            focalDepth += 5;
        }
    } else if (ch == ',' || ch == '<') {
        if (fstop > 1.2) {
            fstop -= 0.1;
        }
    } else if (ch == '.' || ch == '>') {
        if (fstop < 16.0) {
            fstop += 0.1;
        }
    }

    //  Reproject
    Project(fov(focalLength), asp, 1);
    //  Tell GLUT it is necessary to redisplay the scene
    glutPostRedisplay();
}

/*
 *  GLUT calls this routine when the window is resized
 */
void reshape(int width, int height)
{
    screen_width = width;
    screen_height = height;

    //  Ratio of the width to the height of the window
    asp = (height > 0) ? (double) width / height : 1;
    //  Set the viewport to the entire window
    glViewport(0, 0, width, height);
    //  Set projection
    Project(fov(focalLength), asp, 1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_width, screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, screen_width, screen_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width, screen_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

/*
 *  Read text file
 */
char *ReadText(char *file)
{
    size_t n;
    char *buffer;
    //  Open file
    FILE *f = fopen(file, "rt");
    if (!f) Fatal("Cannot open text file %s\n", file);
    //  Seek to end to determine size, then rewind
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    rewind(f);
    //  Allocate memory for the whole file
    buffer = (char *) malloc(n + 1);
    if (!buffer) Fatal("Cannot allocate %d bytes for text file %s\n", n + 1, file);
    //  Snarf the file
    if (fread(buffer, n, 1, f) != 1) Fatal("Cannot read %d bytes for text file %s\n", n, file);
    buffer[n] = 0;
    //  Close and return
    fclose(f);
    return buffer;
}

/*
 *  Print Shader Log
 */
void PrintShaderLog(GLuint obj, char *file)
{
    int len = 0;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        int n = 0;
        char *buffer = (char *) malloc(len);
        if (!buffer) Fatal("Cannot allocate %d bytes of text for shader log\n", len);
        glGetShaderInfoLog(obj, len, &n, buffer);
        fprintf(stderr, "%s:\n%s\n", file, buffer);
        free(buffer);
    }
    glGetShaderiv(obj, GL_COMPILE_STATUS, &len);
    if (!len) Fatal("Error compiling %s\n", file);
}

/*
 *  Print Program Log
 */
void PrintProgramLog(GLuint obj)
{
    int len = 0;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        int n = 0;
        char *buffer = (char *) malloc(len);
        if (!buffer) Fatal("Cannot allocate %d bytes of text for program log\n", len);
        glGetProgramInfoLog(obj, len, &n, buffer);
        fprintf(stderr, "%s\n", buffer);
    }
    glGetProgramiv(obj, GL_LINK_STATUS, &len);
    if (!len) Fatal("Error linking program\n");
}

/*
 *  Create Shader
 */
GLuint CreateShader(GLenum type, char *file)
{
    //  Create the shader
    GLuint shader = glCreateShader(type);
    //  Load source code from file
    char *source = ReadText(file);
    glShaderSource(shader, 1, (const char **) &source, NULL);
    free(source);
    //  Compile the shader
    fprintf(stderr, "Compile %s\n", file);
    glCompileShader(shader);
    //  Check for errors
    PrintShaderLog(shader, file);
    //  Return name
    return shader;
}

/*
 *  Create Shader Program
 */
GLuint CreateShaderProg(char *VertFile, char *FragFile)
{
    //  Create program
    GLuint prog = glCreateProgram();
    //  Create and compile vertex shader
    GLuint vert = CreateShader(GL_VERTEX_SHADER, VertFile);
    //  Create and compile fragment shader
    GLuint frag = CreateShader(GL_FRAGMENT_SHADER, FragFile);
    //  Attach vertex shader
    glAttachShader(prog, vert);
    //  Attach fragment shader
    glAttachShader(prog, frag);
    //  Link program
    glLinkProgram(prog);
    //  Check for errors
    PrintProgramLog(prog);
    //  Return name
    return prog;
}

/*
 *  Start up GLUT and tell it what to do
 */
int main(int argc, char *argv[])
{
    //  Initialize GLUT
    glutInit(&argc, argv);
    //  Request double buffered, true color window with Z buffering at 600x600
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(screen_width, screen_height);
    glutCreateWindow("Shaders");
#ifdef USEGLEW
    //  Initialize GLEW
    if (glewInit() != GLEW_OK) Fatal("Error initializing GLEW\n");
#endif

    init_resources();

    //  Set callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(key);
    glutIdleFunc(idle);

    //  Load object
    tank = LoadOBJ("./models/tank/tank.obj");

    //  Create Shader Programs
    texture_shader = CreateShaderProg("texture.vert", "texture.frag");
    postproc_shader = CreateShaderProg("postproc.vert", "postproc.frag");

    attribute_v_coord = glGetAttribLocation(postproc_shader, "v_coord");
    uniform_color_texture = glGetUniformLocation(postproc_shader, "color_texture");
    uniform_depth_texture = glGetUniformLocation(postproc_shader, "depth_texture");
    uniform_focal_depth = glGetUniformLocation(postproc_shader, "focalDepth");
    uniform_focal_length = glGetUniformLocation(postproc_shader, "focalLength");
    uniform_fstop = glGetUniformLocation(postproc_shader, "fstop");
    uniform_screen_width = glGetUniformLocation(postproc_shader, "width");
    uniform_screen_height = glGetUniformLocation(postproc_shader, "height");

    //  Pass control to GLUT so it can interact with the user
    ErrCheck("init");
    glutMainLoop();

    free_resources();
    return 0;
}
