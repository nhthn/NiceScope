#include "main.hpp"

constexpr int k_bufferSize = 256;

const int k_maxMessageLength = 1024;

static int g_windowWidth = 640;
static int g_windowHeight = 480;

GLFWwindow* setUpWindowAndOpenGL(const char* windowTitle) {
    if (!glfwInit()) {
        throw std::runtime_error("GLFW initialization failed.");
    }
    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight, windowTitle, NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("GLFW initialization failed.");
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();
    if (!glCreateShader) {
        throw std::runtime_error("Unsuccessful GLEW initialization.");
    }

    return window;
}

class MinimalOpenGLApp {
public:
    MinimalOpenGLApp(GLFWwindow* window)
    {
        m_callback = new VisualizerAudioCallback();
        m_window = window;
        glfwSetFramebufferSizeCallback(m_window, resize);
    }

    void setProgram(GLuint program)
    {
        m_program = program;
    }

    void setNumTriangles(int numTriangles)
    {
        m_numTriangles = numTriangles;
    }

    AudioCallback* getAudioCallback()
    {
        return m_callback;
    }

private:
    static constexpr int m_size = k_bufferSize;
    float m_array[m_size];

    const char* m_windowTitle;
    int m_numTriangles;
    GLFWwindow* m_window;
    GLuint m_program;

    VisualizerAudioCallback* m_callback;

    void render()
    {
    }

    static void resize(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        g_windowWidth = width;
        g_windowHeight = height;
    }
};

class ShaderProgram {
public:
    ShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
        : m_vertexShaderSource(vertexShaderSource)
        , m_fragmentShaderSource(fragmentShaderSource)
    {
        makeVertexShader();
        makeFragmentShader();
        makeProgram();
    }

    ~ShaderProgram()
    {
        cleanUp();
    }

    GLuint getProgram()
    {
        return m_program;
    }

    GLuint getAttribLocation(const char* key)
    {
        return glGetAttribLocation(m_program, key);
    }

private:
    const char* m_vertexShaderSource;
    const char* m_fragmentShaderSource;
    GLuint m_vertexShader;
    GLuint m_fragmentShader;
    GLuint m_program;

    void makeVertexShader()
    {
        m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_vertexShader, 1, &m_vertexShaderSource, NULL);
        glCompileShader(m_vertexShader);

        GLint success;
        glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE) {
            GLsizei logLength = 0;
            GLchar c_message[k_maxMessageLength];
            glGetShaderInfoLog(m_vertexShader, k_maxMessageLength, &logLength, c_message);
            std::string message = c_message;
            std::string full_message = "Error compiling vertex shader: " + message;
            throw std::runtime_error(full_message);
        }
    }

    void makeFragmentShader()
    {
        m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_fragmentShader, 1, &m_fragmentShaderSource, NULL);
        glCompileShader(m_fragmentShader);

        GLint success;
        glGetShaderiv(m_fragmentShader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE) {
            GLsizei logLength = 0;
            GLchar c_message[k_maxMessageLength];
            glGetShaderInfoLog(m_fragmentShader, k_maxMessageLength, &logLength, c_message);
            std::string message = c_message;
            std::string full_message = "Error compiling fragment shader: " + message;
            throw std::runtime_error(full_message);
        }
    }

    void makeProgram()
    {
        m_program = glCreateProgram();
        glAttachShader(m_program, m_vertexShader);
        glAttachShader(m_program, m_fragmentShader);
        glLinkProgram(m_program);
    }

    void cleanUp()
    {
        glDetachShader(m_program, m_vertexShader);
        glDetachShader(m_program, m_fragmentShader);
        glDeleteProgram(m_program);
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_fragmentShader);
    }
};

class Rectangle {
public:
    Rectangle()
    {
        const char* vertexShaderSource = ("#version 130\n"
                                          "in vec2 pos;\n"
                                          "void main()\n"
                                          "{\n"
                                          "    gl_Position = vec4(pos, 1, 1);\n"
                                          "}\n");

        const char* fragmentShaderSource = (
            "#version 130\n"
            "uniform vec2 windowSize;\n"
            "out vec3 fragColor;\n"
            "void main()\n"
            "{\n"
            "fragColor = vec3(1.0);\n"
            "}\n"
        );

        ShaderProgram shaderProgram(vertexShaderSource, fragmentShaderSource);
        m_program = shaderProgram.getProgram();

        makeVertexBuffer();
        makeArrayBuffer();
        makeElementBuffer();
    }

    ~Rectangle()
    {
        cleanUp();
    }

    int getProgram()
    {
        return m_program;
    }

    int getNumTriangles()
    {
        return m_numTriangles;
    }

    void render() {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(m_program);

        GLuint color = glGetUniformLocation(m_program, "windowSize");
        glUniform2f(color, g_windowWidth, g_windowHeight);

        glDrawElements(GL_TRIANGLES, 3 * m_numTriangles, GL_UNSIGNED_INT, (void*)0);
    }

private:
    int m_numTriangles = 2;
    GLuint m_program;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLfloat m_coordinates[8] = { -1, -1, -1, 1, 1, -1, 1, 1 };
    GLuint m_elements[6] = { 0, 1, 2, 1, 2, 3 };

    void makeVertexBuffer()
    {
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(m_coordinates), m_coordinates, GL_STATIC_DRAW);
    }

    void makeArrayBuffer()
    {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glVertexAttribPointer(glGetAttribLocation(m_program, "pos"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void makeElementBuffer()
    {
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_elements), m_elements, GL_STATIC_DRAW);
    }

    void cleanUp()
    {
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &m_ebo);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &m_vbo);

        glBindVertexArray(0);
        glDeleteVertexArrays(1, &m_vao);
    }
};

VisualizerAudioCallback::VisualizerAudioCallback()
    : m_bufferSize(k_bufferSize)
{
    m_writePos = 0;
    m_buffer = static_cast<float*>(malloc(sizeof(float) * m_bufferSize));
    for (int i = 0; i < m_bufferSize; i++) {
        m_buffer[i] = 0;
    }
}

VisualizerAudioCallback::~VisualizerAudioCallback()
{
    free(m_buffer);
}

void VisualizerAudioCallback::process(InputBuffer input_buffer, OutputBuffer output_buffer, int frame_count)
{
    for (int i = 0; i < frame_count; i++) {
        m_buffer[m_writePos] = input_buffer[0][i];
        m_writePos += 1;
        if (m_writePos == m_bufferSize) {
            m_writePos = 0;
        }
    }
}

int main(int argc, char** argv)
{
    auto window = setUpWindowAndOpenGL("Scope");
    MinimalOpenGLApp app(window);

    PortAudioBackend audioBackend(app.getAudioCallback());
    audioBackend.run();

    Rectangle rectangle;

    while (!glfwWindowShouldClose(window)) {
        rectangle.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
