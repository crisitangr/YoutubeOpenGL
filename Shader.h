#pragma once
#include <glad/glad.h>
#include <iostream>

class Shader
{
public:
    // Identificador del programa de shaders
    GLuint ID;

    // Compila y enlaza los shaders (crear despues de gladLoadGL)
    Shader()
    {
        // Vertex shader
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec4 aColor;\n"
            "out vec4 vColor;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "   vColor = aColor;\n"
            "}\0";

        // Fragment shader
        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec4 vColor;\n"
            "out vec4 FragColor;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vColor;\n"
            "}\n\0";

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        RevisarShader(vertexShader, "VERTEX");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        RevisarShader(fragmentShader, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        RevisarPrograma(ID);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    // Activa el programa
    void Usar()
    {
        glUseProgram(ID);
    }

    // Borra el programa
    void Borrar()
    {
        glDeleteProgram(ID);
    }

private:
    // Muestra el error de compilacion, si hay
    void RevisarShader(GLuint shader, const char* tipo)
    {
        GLint exito;
        char info[1024];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &exito);
        if (!exito)
        {
            glGetShaderInfoLog(shader, 1024, NULL, info);
            std::cout << "Error al compilar el shader " << tipo << ":\n" << info << std::endl;
        }
    }

    // Muestra el error de enlace, si hay
    void RevisarPrograma(GLuint programa)
    {
        GLint exito;
        char info[1024];
        glGetProgramiv(programa, GL_LINK_STATUS, &exito);
        if (!exito)
        {
            glGetProgramInfoLog(programa, 1024, NULL, info);
            std::cout << "Error al enlazar el programa de shaders:\n" << info << std::endl;
        }
    }
};