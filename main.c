/*
 *  Shaders
 *
 *  Demonstrate shaders:
 *    Simple shaders
 *    Vertex lighting
 *    Procedural textures
 *    Toon shader
 *    Pixel lighting
 *    Textured lookup
 *
 *  Key bindings:
 *  m/M        Cycle through shaders
 *  x/X        Increase/decrease mandelbrot X-value
 *  y/Y        Increase/decrease mandelbrot Y-value
 *  z/Z        Zoom in/out of mandelbrot
 *  o/O        Cycle through objects
 *  p/P        Toggle between orthogonal & perspective projections
 *  s/S        Start/stop light movement
 *  -/+        Decrease/increase light elevation
 *  a          Toggle axes
 *  arrows     Change view angle
 *  PgDn/PgUp  Zoom in and out
 *  0          Reset view angle
 *  ESC        Exit
 */

#include "CSCIx229.h"

int axes = 1;       //  Display axes
int mode = 6;       //  Shader mode
int move = 1;       //  Move light
int roll = 1;       //  Rolling brick texture
int proj = 1;       //  Projection type
int obj = 1;        //  Object
int th = 0;         //  Azimuth of view angle
int ph = 0;         //  Elevation of view angle
int fov = 55;       //  Field of view (for perspective)
double asp = 1;     //  Aspect ratio
double dim = 3.0;   //  Size of world
int zh = 90;        //  Light azimuth
int tank = 0;       //  Object
float Ylight = 2;   //  Light elevation
#define MODE 7
GLuint shader[MODE] = {0, 0, 0, 0, 0, 0, 0}; //  Shader programs
char *text[] = {"Constant Color", "Lighting", "Brick", "Mandelbrot Set", "Toon Shader", "Pixel Lighting",
                "Textures"};
float X = 0, Y = 0, Z = 1; //  Mandelbrot X,Y,Z

GLuint post_shader;
GLuint attribute_v_coord, uniform_fbo_texture;

GLuint fbo, fbo_texture, rbo_depth;
GLuint vbo_fbo_vertices;

int screen_width = 800, screen_height = 600;

int init_resources()
{
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &fbo_texture);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_width, screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);

    /* Depth buffer */
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, screen_width, screen_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    /* Framebuffer to link everything together */
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);
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
    return 1;
}

void free_resources()
{
    glDeleteRenderbuffers(1, &rbo_depth);
    glDeleteTextures(1, &fbo_texture);
    glDeleteFramebuffers(1, &fbo);
    glDeleteBuffers(1, &vbo_fbo_vertices);
    glDeleteProgram(post_shader);
}

/*
 *  OpenGL (GLUT) calls this routine to display the scene
 */
void display()
{
    //  Light position and colors
    float Emission[] = {0.0, 0.0, 0.0, 1.0};
    float Ambient[] = {0.3, 0.3, 0.3, 1.0};
    float Diffuse[] = {1.0, 1.0, 1.0, 1.0};
    float Specular[] = {1.0, 1.0, 1.0, 1.0};
    float Position[] = {2 * Cos(zh), Ylight, 2 * Sin(zh), 1.0};
    float Shinyness[] = {16};

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //  Erase the window and the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //  Enable Z-buffering in OpenGL
    glEnable(GL_DEPTH_TEST);
    //  Undo previous transformations
    glLoadIdentity();
    //  Perspective - set eye position
    if (proj) {
        double Ex = -2 * dim * Sin(th) * Cos(ph);
        double Ey = +2 * dim * Sin(ph);
        double Ez = +2 * dim * Cos(th) * Cos(ph);
        gluLookAt(Ex, Ey, Ez, 0, 0, 0, 0, Cos(ph), 0);
    }
        //  Orthogonal - set world orientation
    else {
        glRotatef(ph, 1, 0, 0);
        glRotatef(th, 0, 1, 0);
    }

    //  Draw light position as sphere (still no lighting here)
    glColor3f(1, 1, 1);
    glPushMatrix();
    glTranslated(Position[0], Position[1], Position[2]);
    glutSolidSphere(0.03, 10, 10);
    glPopMatrix();
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
    glUseProgram(shader[mode]);
    //  For brick shader set "uniform" variables
    if (mode > 0) {
        int id;
        float time = roll ? 0.001 * glutGet(GLUT_ELAPSED_TIME) : 0;
        id = glGetUniformLocation(shader[mode], "Xcenter");
        if (id >= 0) glUniform1f(id, X);
        id = glGetUniformLocation(shader[mode], "Ycenter");
        if (id >= 0) glUniform1f(id, Y);
        id = glGetUniformLocation(shader[mode], "Zoom");
        if (id >= 0) glUniform1f(id, Z);
        id = glGetUniformLocation(shader[mode], "time");
        if (id >= 0) glUniform1f(id, time);
    }
    //  Draw the teapot or cube
    glColor3f(0, 1, 1);
    glEnable(GL_TEXTURE_2D);
    glCallList(tank);
    glDisable(GL_TEXTURE_2D);

    /* Post-processing */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(post_shader);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glUniform1i(uniform_fbo_texture, /*GL_TEXTURE*/1);
    glDisable(GL_TEXTURE_2D);
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

    //  Display parameters
    glWindowPos2i(5, 5);
    Print("Angle=%d,%d  Dim=%.1f Projection=%s Mode=%s",
          th, ph, dim, proj ? "Perpective" : "Orthogonal", text[mode]);
    if (mode == 4) {
        glWindowPos2i(5, 25);
        Print("X=%f Y=%f Z=%f Mag=%f", X, Y, Z, 1 / Z);
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
    //  Elapsed time in seconds
    double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    if (move) zh = fmod(90 * t, 360.0);
    //  Tell GLUT it is necessary to redisplay the scene
    glutPostRedisplay();
}

/*
 *  GLUT calls this routine when an arrow key is pressed
 */
void special(int key, int x, int y)
{
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
        //  PageUp key - increase dim
    else if (key == GLUT_KEY_PAGE_DOWN)
        dim += 0.1;
        //  PageDown key - decrease dim
    else if (key == GLUT_KEY_PAGE_UP && dim > 1)
        dim -= 0.1;
    //  Keep angles to +/-360 degrees
    th %= 360;
    ph %= 360;
    //  Update projection
    Project(proj ? fov : 0, asp, dim);
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
        //  Reset view angle
    else if (ch == '0')
        th = ph = 0;
        //  Toggle axes
    else if (ch == 'a' || ch == 'A')
        axes = 1 - axes;
        //  Toggle projection type
    else if (ch == 'p' || ch == 'P')
        proj = 1 - proj;
        //  Toggle light movement
    else if (ch == 's' || ch == 'S')
        move = 1 - move;
        //  Toggle brick movement
    else if (ch == 'b' || ch == 'B')
        roll = 1 - roll;
        //  Toggle objects
    else if (ch == 'o' || ch == 'O')
        obj = (obj + 1) % 2;
        //  Cycle modes
    else if (ch == 'm')
        mode = (mode + 1) % MODE;
    else if (ch == 'M')
        mode = (mode + MODE - 1) % MODE;
        //  Light elevation
    else if (ch == '+')
        Ylight += 0.1;
    else if (ch == '-')
        Ylight -= 0.1;
        //  Mandelbrot
    else if (ch == 'x')
        X += 0.01 * Z;
    else if (ch == 'X')
        X -= 0.01 * Z;
    else if (ch == 'y')
        Y += 0.01 * Z;
    else if (ch == 'Y')
        Y -= 0.01 * Z;
    else if (ch == 'z')
        Z *= 0.9;
    else if (ch == 'Z')
        Z *= 1.1;
    else if (ch == 'r' || ch == 'R') {
        X = Y = 0;
        Z = 1;
    }
    //  Reproject
    Project(proj ? fov : 0, asp, dim);
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
    Project(proj ? fov : 0, asp, dim);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_width, screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
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
    tank = LoadOBJ(argv[1]);

    //  Create Shader Programs
    shader[0] = CreateShaderProg("simple.vert", "simple.frag");
    shader[1] = CreateShaderProg("phong.vert", "phong.frag");
    shader[2] = CreateShaderProg("model.vert", "brick.frag");
    shader[3] = CreateShaderProg("model.vert", "mandel.frag");
    shader[4] = CreateShaderProg("toon.vert", "toon.frag");
    shader[5] = CreateShaderProg("pixlight.vert", "pixlight.frag");
    shader[6] = CreateShaderProg("texture.vert", "texture.frag");

    post_shader = CreateShaderProg("postproc.v.glsl", "postproc.f.glsl");

    attribute_v_coord = glGetAttribLocation(post_shader, "v_coord");
    if (attribute_v_coord == -1) {
        fprintf(stderr, "Could not bind attribute v_coord\n");
        return 0;
    }

    uniform_fbo_texture = glGetUniformLocation(post_shader, "fbo_texture");
    if (uniform_fbo_texture == -1) {
        fprintf(stderr, "Could not bind uniform fbo_texture\n");
        return 0;
    }

    //  Pass control to GLUT so it can interact with the user
    ErrCheck("init");
    glutMainLoop();

    free_resources();
    return 0;
}
