#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct VertexSquare {
    GLfloat pos[3];
    GLfloat color[4];
};

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec4 aColor;\n"
"out vec4 vColor;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"   vColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec4 vColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vColor;\n"
"}\n\0";

const int GRILLA_COLUMNAS = 50;
const int GRILLA_FILAS = 50;
const int TAMANO_VENTANA = 800;

GLfloat colorGrilla[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
GLfloat colorFigura[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float margen = 0.1f;

std::vector<VertexSquare> verticesGrilla;
std::vector<GLuint> indicesGrilla;

enum Modo { MODO_LINEA, MODO_CIRCULO };
Modo modoActual = MODO_LINEA;
bool tienePrimerPunto = false;
int primerX = 0, primerY = 0;

GLuint g_VBO = 0;

void CrearCuadradosGrilla(std::vector<VertexSquare>& outVerts, std::vector<GLuint>& outIndices, int columnas, int filas, const GLfloat color[4])
{
    outVerts.clear();
    outIndices.clear();

    float anchoCelda = 2.0f / columnas;
    float altoCelda = 2.0f / filas;
    float inicioX = -1.0f;
    float inicioY = -1.0f;

    for (int y = 0; y < filas; ++y)
    {
        for (int x = 0; x < columnas; ++x)
        {
            float centroX = inicioX + (x + 0.5f) * anchoCelda;
            float centroY = inicioY + (y + 0.5f) * altoCelda;

            float mitadAncho = (anchoCelda * 0.5f) * (1.0f - margen);
            float mitadAlto = (altoCelda * 0.5f) * (1.0f - margen);

            VertexSquare v0 = { { centroX + mitadAncho, centroY + mitadAlto, 0.0f }, { color[0], color[1], color[2], color[3] } };
            VertexSquare v1 = { { centroX + mitadAncho, centroY - mitadAlto, 0.0f }, { color[0], color[1], color[2], color[3] } };
            VertexSquare v2 = { { centroX - mitadAncho, centroY - mitadAlto, 0.0f }, { color[0], color[1], color[2], color[3] } };
            VertexSquare v3 = { { centroX - mitadAncho, centroY + mitadAlto, 0.0f }, { color[0], color[1], color[2], color[3] } };

            GLuint base = static_cast<GLuint>(outVerts.size());
            outVerts.push_back(v0);
            outVerts.push_back(v1);
            outVerts.push_back(v2);
            outVerts.push_back(v3);

            outIndices.push_back(base + 0);
            outIndices.push_back(base + 1);
            outIndices.push_back(base + 2);
            outIndices.push_back(base + 0);
            outIndices.push_back(base + 2);
            outIndices.push_back(base + 3);
        }
    }
}

void FijarColorCelda(int celdaX, int celdaY, const GLfloat color[4])
{
    if (celdaX < 0 || celdaX >= GRILLA_COLUMNAS || celdaY < 0 || celdaY >= GRILLA_FILAS) return;

    size_t indiceCelda = static_cast<size_t>(celdaY) * GRILLA_COLUMNAS + celdaX;
    size_t base = indiceCelda * 4;

    for (size_t i = 0; i < 4; ++i)
    {
        verticesGrilla[base + i].color[0] = color[0];
        verticesGrilla[base + i].color[1] = color[1];
        verticesGrilla[base + i].color[2] = color[2];
        verticesGrilla[base + i].color[3] = color[3];
    }
}

void SubirGrillaGPU()
{
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verticesGrilla.size() * sizeof(VertexSquare), verticesGrilla.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void DibujarLineaH(int x0, int y0, int x1, int y1, const GLfloat color[4])
{
    if (x0 > x1) {
        int temp = x0;
        x0 = x1;
        x1 = temp;

        temp = y0;
        y0 = y1;
        y1 = temp;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int dir = (dy < 0) ? -1 : 1;
    dy *= dir;

    if (dx != 0)
    {
        int y = y0;
        int p = 2 * dy - dx;   

        for (int i = 0; i <= dx; ++i)
        {
            FijarColorCelda(x0 + i, y, color);

            if (p >= 0)
            {
                y += dir;
                p -= 2 * dx;
            }
            p += 2 * dy;
        }
    }
}

void DibujarLineaV(int x0, int y0, int x1, int y1, const GLfloat color[4])
{
    if (y0 > y1) {
        int temp = x0;
        x0 = x1;
        x1 = temp;

        temp = y0;
        y0 = y1;
        y1 = temp;
    }

    int dx = x1 - x0;
    int dy = y1 - y0;

    int dir = (dx < 0) ? -1 : 1;
    dx *= dir;

    if (dy != 0)
    {
        int x = x0;
        int p = 2 * dx - dy;   

        for (int i = 0; i <= dy; ++i)
        {
            FijarColorCelda(x, y0 + i, color);

            if (p >= 0)
            {
                x += dir;
                p -= 2 * dy;
            }
            p += 2 * dx;
        }
    }
}

void DibujarLinea(int x0, int y0, int x1, int y1, const GLfloat color[4])
{
    if (x0 == x1 && y0 == y1)
    {
        FijarColorCelda(x0, y0, color);   
        return;
    }

    if (std::abs(x1 - x0) > std::abs(y1 - y0))
        DibujarLineaH(x0, y0, x1, y1, color);
    else
        DibujarLineaV(x0, y0, x1, y1, color);
}


void PintarOctantes(int cx, int cy, int x, int y, const GLfloat color[4])
{
    FijarColorCelda(cx + x, cy + y, color);
    FijarColorCelda(cx - x, cy + y, color);
    FijarColorCelda(cx + x, cy - y, color);
    FijarColorCelda(cx - x, cy - y, color);
    FijarColorCelda(cx + y, cy + x, color);
    FijarColorCelda(cx - y, cy + x, color);
    FijarColorCelda(cx + y, cy - x, color);
    FijarColorCelda(cx - y, cy - x, color);
}

void DibujarCirculo(int cx, int cy, int r, const GLfloat color[4])
{
    if (r <= 0)
    {
        FijarColorCelda(cx, cy, color);   // un solo punto
        return;
    }

    int x = 0;
    int y = r;
    int p = 1 - r;   

    while (x <= y)
    {
        PintarOctantes(cx, cy, x, y, color);

        if (p >= 0)
        {
            y -= 1;
            p -= 2 * y;
        }
        x += 1;
        p += 2 * x + 1;
    }
}

void LimpiarGrilla()
{
    for (int y = 0; y < GRILLA_FILAS; ++y)
        for (int x = 0; x < GRILLA_COLUMNAS; ++x)
            FijarColorCelda(x, y, colorGrilla);
    tienePrimerPunto = false;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int anchoVentana, altoVentana;
    glfwGetWindowSize(window, &anchoVentana, &altoVentana);

    int columna = static_cast<int>(xpos / anchoVentana * GRILLA_COLUMNAS);
    int fila = static_cast<int>((altoVentana - ypos) / altoVentana * GRILLA_FILAS);

    if (columna < 0) columna = 0;
    if (columna >= GRILLA_COLUMNAS) columna = GRILLA_COLUMNAS - 1;
    if (fila < 0) fila = 0;
    if (fila >= GRILLA_FILAS) fila = GRILLA_FILAS - 1;

    if (!tienePrimerPunto)
    {
        primerX = columna;
        primerY = fila;
        tienePrimerPunto = true;
    }
    else
    {
        if (modoActual == MODO_LINEA)
        {
            DibujarLinea(primerX, primerY, columna, fila, colorFigura);
        }
        else
        {
            int dx = columna - primerX;
            int dy = fila - primerY;
            int r = static_cast<int>(std::round(std::sqrt(static_cast<double>(dx * dx + dy * dy))));
            DibujarCirculo(primerX, primerY, r, colorFigura);
        }
        tienePrimerPunto = false;
    }

    SubirGrillaGPU();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_L || key == GLFW_KEY_C)
    {
        tienePrimerPunto = false;

        if (key == GLFW_KEY_L) {
            modoActual = MODO_LINEA;
            glfwSetWindowTitle(window, "Paint - Linea");
        }
        else {
            modoActual = MODO_CIRCULO;
            glfwSetWindowTitle(window, "Paint - Circulo");
        }
    }
    else if (key == GLFW_KEY_R)
    {
        LimpiarGrilla();
        SubirGrillaGPU();
    }
}

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(TAMANO_VENTANA, TAMANO_VENTANA, "Paint - Linea", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Error al crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glViewport(0, 0, TAMANO_VENTANA, TAMANO_VENTANA);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);

  
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    CrearCuadradosGrilla(verticesGrilla, indicesGrilla, GRILLA_COLUMNAS, GRILLA_FILAS, colorGrilla);

    GLuint VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &g_VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesGrilla.size() * sizeof(VertexSquare), verticesGrilla.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesGrilla.size() * sizeof(GLuint), indicesGrilla.data(), GL_STATIC_DRAW);

    GLsizei stride = sizeof(VertexSquare);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexSquare, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(VertexSquare, color));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    GLsizei cantidadIndices = static_cast<GLsizei>(indicesGrilla.size());

    while (!glfwWindowShouldClose(window))
    {
    
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Herramientas");

        if (ImGui::Button("Linea"))
        {
            modoActual = MODO_LINEA;
            tienePrimerPunto = false;
            glfwSetWindowTitle(window, "Paint - Linea");
        }
        ImGui::SameLine();
        if (ImGui::Button("Circulo"))
        {
            modoActual = MODO_CIRCULO;
            tienePrimerPunto = false;
            glfwSetWindowTitle(window, "Paint - Circulo");
        }

        ImGui::ColorEdit4("Color", colorFigura);

        if (ImGui::Button("Limpiar"))
        {
            LimpiarGrilla();
            SubirGrillaGPU();
        }

        ImGui::End();
        ImGui::Render();

        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, cantidadIndices, GL_UNSIGNED_INT, 0);

     
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}