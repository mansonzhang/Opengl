#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <string>
#include <sstream>

#include <fstream>

//断言宏  __ 双下划线代表是该编译器内部的,不会在Clang/GCC等其它编译器工作
#define ASSERT(x) if(!x) __debugbreak();
//宏换行，需要紧接着"\"
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x,__FILE__,__LINE__))
static void GLClearError() 
{
    while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall()
{
    while (GLenum error=glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << ")\n";
        return false;
    }
    return true;
}
static bool GLLogCall(const char* function,const char* file,int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "[OpenGL Error] (" << error << ") : in "<<function<<" "<<file<<" at "<<line<<" line\n";
        return false;
    }
    return true;
}
struct ShaderProgramSources {
    std::string VertexSource;
    std::string FragmentSource;
};

static ShaderProgramSources ParseShader(const std::string& filepath )
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];//字符串流
    ShaderType curtype = ShaderType::NONE;
    while (getline(stream,line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos) 
            {
                //set mode to vertex
                curtype = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos) 
            {
                //set mode to fragment
                curtype = ShaderType::FRAGMENT;
            }
        }
        else
        {
            ss[(int)curtype] << line << "\n";
        }
    }
    return{ ss[0].str(),ss[1].str() };
}

static GLuint CompileShader(GLuint type, const std::string& source) {
    GLuint id = glCreateShader(type);//创建顶点着色器类型的shader
    const char* src = source.c_str();//或者写作 &source[0]
    glShaderSource(id,1,&src,nullptr);
    glCompileShader(id);
    //TODO: Error handling
    int result;
    glGetShaderiv(id,GL_COMPILE_STATUS,&result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));//alloca是C语言函数，在栈上动态分配
        glGetShaderInfoLog(id,length, &length,message);
        std::cout <<"Failed to compile"<< (type==GL_VERTEX_SHADER?"vertex":"fragment") << "shader" << message << "\n";
        glDeleteShader(id);
        return 0;
    }
    return id;
}
static GLuint CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    GLuint program = glCreateProgram();
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);//顶点着色器
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);//片段着色器

    //应用着色器
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    //链接着色器到程序
    glLinkProgram(program);
    glValidateProgram(program);

    //删除着色器中间部分(因为着色器已经被链接到程序中了),类似于编译成 .cpp ->.obj,已经有地方存储着色器了
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello Manson", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);//maybe 同步刷新频率

    GLenum  err=glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        std::cout << "Error!\n";
    }
    std::cout << glGetString(GL_VERSION)<<"\n"; 
    /* Loop until the user closes the window */

    float positions[] = {
        -0.5f,-0.5f,
        -0.5f, 0.5f,
        0.5f, 0.5f,
        0.5f, -0.5f
    };
    unsigned int indices[] = {
        0,1,2,0,3,2
    };
    unsigned int buffer;
    glGenBuffers(1,&buffer);
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glBufferData(GL_ARRAY_BUFFER,4*2*sizeof(float), positions,GL_STATIC_DRAW);

    unsigned int ibo;//索引缓冲 index buffer object
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6  * sizeof(unsigned int), indices, GL_STATIC_DRAW);//提供indices索引

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(float)*2,0);
    //2->2个数据标识一个点
    //sizeof(float)*2->positions步行到下一个点位的步幅 -0.5f,-0.5f=》0, 0.5f

    std::string shaderFilePath = "res/shaders/Basic.shader";
    ShaderProgramSources shaderSource = ParseShader(shaderFilePath);
    GLuint program = CreateShader(shaderSource.VertexSource, shaderSource.FragmentSource);
    GLCall(glUseProgram(program));

    GLCall(int location = glGetUniformLocation(program,"u_Color"));
    //ASSERT(location != -1);
    GLCall(glUniform4f(location,0.2f,0.8f,0.8f,1.0f));

    float r = 0.0f;
    float increment = 0.05;
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        //(0)原始的流水线画法
        //glBegin(GL_TRIANGLES);
        //glVertex2f(-0.5f,-0.5f);
        //glVertex2f(0, 0.5f);
        //glVertex2f(0.5f, -0.5f);
        //glEnd();

        ///(1)不使用顶点缓冲区
        //glDrawArrays(GL_TRIANGLES,0,3);//画出三角形
        //glDrawArrays(GL_TRIANGLES, 0, 6);//画出正方形->也就是2个三角形拼成
        
        //(2)使用顶点缓冲区
        //GLClearError();
        //glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,nullptr);//使用顶点缓冲区画正方形,6是指索引数量
        //ASSERT(GLLogCall());

        //(3)使用宏封装，简化调用OpenGL日志的写法
        GLCall(glUniform4f(location, r, 0.8f, 0.8f, 1.0f));
        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
        if (r > 1.0f) {
            increment = -0.01f;
        }
        else if (r < 0) {
            increment = 0.01f;
        }
        r += increment;
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }
    
    glDeleteProgram(program);//清理程序
    
    glfwTerminate();
    return 0;
}