#include <iostream>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Shader.h"

struct VertexSquare {
    GLfloat pos[3];
    GLfloat color[4];
};

struct Punto {
    int x;
    int y;
};

enum Modo { MODO_LINEA, MODO_CIRCULO, MODO_LIBRE, MODO_TRIANGULO, MODO_CUADRADO };
const char* nombresModo[] = { "Linea", "Circulo", "Libre", "Triangulo", "Cuadrado" };

// Figura dibujada: guarda los clics, no los pixeles
struct Figura {
    Modo tipo;
    std::vector<Punto> pts;
    bool relleno;
    GLfloat colorFigura[4];
    GLfloat colorRelleno[4];
    float angulo;
};

const int GRILLA_COLUMNAS = 50;
const int GRILLA_FILAS = 50;
const int TAMANO_VENTANA = 800;
const float PI = 3.14159265f;

GLfloat colorGrilla[4] = { 0.22f, 0.20f, 0.32f, 1.0f };
GLfloat colorFigura[4] = { 0.0f, 0.9f, 0.75f, 1.0f };
GLfloat colorRelleno[4] = { 0.95f, 0.35f, 0.65f, 1.0f };
GLfloat colorFondo[4] = { 0.10f, 0.06f, 0.16f, 1.0f };
GLfloat colorPunto[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float margen = 0.15f;

std::vector<VertexSquare> verticesGrilla;
std::vector<GLuint> indicesGrilla;

std::vector<Figura> figuras;
std::vector<Punto> clics;
Modo modoActual = MODO_LINEA;
bool rellenar = false;
bool dibujandoLibre = false;

GLuint g_VBO = 0;

// Grilla: vertices e indices de las celdas
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

// putPixel: pinta una celda
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

// Envia los colores a la GPU
void SubirGrillaGPU()
{
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verticesGrilla.size() * sizeof(VertexSquare), verticesGrilla.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Bresenham - linea (parametro de decision p)
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
        // parametro de decision inicial
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
        // parametro de decision inicial
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

// Elige la version segun la pendiente
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

// Bresenham - circulo: 8 puntos simetricos
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

// Bresenham - circulo (parametro de decision p)
void DibujarCirculo(int cx, int cy, int r, const GLfloat color[4])
{
    if (r <= 0)
    {
        FijarColorCelda(cx, cy, color);
        return;
    }

    int x = 0;
    int y = r;
    // parametro de decision inicial
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

// Relleno
void RellenarFila(int xIzq, int xDer, int y, const GLfloat color[4])
{
    for (int x = xIzq; x <= xDer; ++x)
        FijarColorCelda(x, y, color);
}

// Circulo relleno (mismo algoritmo, pinta filas)
void DibujarCirculoRelleno(int cx, int cy, int r, const GLfloat color[4])
{
    if (r <= 0)
    {
        FijarColorCelda(cx, cy, color);
        return;
    }

    int x = 0;
    int y = r;
    // parametro de decision inicial
    int p = 1 - r;

    while (x <= y)
    {
        RellenarFila(cx - x, cx + x, cy + y, color);
        RellenarFila(cx - x, cx + x, cy - y, color);
        RellenarFila(cx - y, cx + y, cy + x, color);
        RellenarFila(cx - y, cx + y, cy - x, color);

        if (p >= 0)
        {
            y -= 1;
            p -= 2 * y;
        }
        x += 1;
        p += 2 * x + 1;
    }
}

// Relleno de poligonos por filas (scanline)
void RellenarPoligono(const Punto v[], int n, const GLfloat color[4])
{
    int yMin = v[0].y;
    int yMax = v[0].y;
    for (int i = 1; i < n; ++i)
    {
        if (v[i].y < yMin) yMin = v[i].y;
        if (v[i].y > yMax) yMax = v[i].y;
    }

    for (int y = yMin; y <= yMax; ++y)
    {
        float xMin = 100000.0f;
        float xMax = -100000.0f;

        for (int i = 0; i < n; ++i)
        {
            Punto a = v[i];
            Punto b = v[(i + 1) % n];

            if (a.y == b.y)
            {
                if (y == a.y)
                {
                    if (a.x < xMin) xMin = static_cast<float>(a.x);
                    if (a.x > xMax) xMax = static_cast<float>(a.x);
                    if (b.x < xMin) xMin = static_cast<float>(b.x);
                    if (b.x > xMax) xMax = static_cast<float>(b.x);
                }
            }
            else if ((y >= a.y && y <= b.y) || (y >= b.y && y <= a.y))
            {
                float x = a.x + static_cast<float>(y - a.y) * (b.x - a.x) / (b.y - a.y);
                if (x < xMin) xMin = x;
                if (x > xMax) xMax = x;
            }
        }

        if (xMin <= xMax)
        {
            int xIzq = static_cast<int>(std::ceil(xMin - 0.0001f));
            int xDer = static_cast<int>(std::floor(xMax + 0.0001f));
            if (xIzq <= xDer) RellenarFila(xIzq, xDer, y, color);
        }
    }
}

// Rotacion: x' = cx + dx*cos - dy*sin ; y' = cy + dx*sin + dy*cos
Punto Rotar(float px, float py, float cx, float cy, float angulo)
{
    float rad = angulo * PI / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);

    float dx = px - cx;
    float dy = py - cy;

    Punto r;
    r.x = static_cast<int>(std::round(cx + dx * c - dy * s));
    r.y = static_cast<int>(std::round(cy + dx * s + dy * c));
    return r;
}

Figura NuevaFigura(Modo tipo)
{
    Figura f;
    f.tipo = tipo;
    f.relleno = rellenar;
    for (int i = 0; i < 4; ++i)
    {
        f.colorFigura[i] = colorFigura[i];
        f.colorRelleno[i] = colorRelleno[i];
    }
    f.angulo = 0.0f;
    return f;
}

// Dibuja una figura con su angulo de giro
void DibujarFigura(const Figura& f)
{
    if (f.pts.empty()) return;

    if (f.tipo == MODO_LINEA)
    {
        float cx = (f.pts[0].x + f.pts[1].x) / 2.0f;
        float cy = (f.pts[0].y + f.pts[1].y) / 2.0f;
        Punto a = Rotar(static_cast<float>(f.pts[0].x), static_cast<float>(f.pts[0].y), cx, cy, f.angulo);
        Punto b = Rotar(static_cast<float>(f.pts[1].x), static_cast<float>(f.pts[1].y), cx, cy, f.angulo);
        DibujarLinea(a.x, a.y, b.x, b.y, f.colorFigura);
    }
    else if (f.tipo == MODO_CIRCULO)
    {
        int dx = f.pts[1].x - f.pts[0].x;
        int dy = f.pts[1].y - f.pts[0].y;
        int r = static_cast<int>(std::round(std::sqrt(static_cast<double>(dx * dx + dy * dy))));

        if (f.relleno) DibujarCirculoRelleno(f.pts[0].x, f.pts[0].y, r, f.colorRelleno);
        DibujarCirculo(f.pts[0].x, f.pts[0].y, r, f.colorFigura);
    }
    else if (f.tipo == MODO_CUADRADO)
    {
        int cx = f.pts[0].x;
        int cy = f.pts[0].y;
        int dx = std::abs(f.pts[1].x - cx);
        int dy = std::abs(f.pts[1].y - cy);
        int h = (dx > dy) ? dx : dy;

        Punto v[4];
        v[0] = Rotar(static_cast<float>(cx - h), static_cast<float>(cy - h), static_cast<float>(cx), static_cast<float>(cy), f.angulo);
        v[1] = Rotar(static_cast<float>(cx + h), static_cast<float>(cy - h), static_cast<float>(cx), static_cast<float>(cy), f.angulo);
        v[2] = Rotar(static_cast<float>(cx + h), static_cast<float>(cy + h), static_cast<float>(cx), static_cast<float>(cy), f.angulo);
        v[3] = Rotar(static_cast<float>(cx - h), static_cast<float>(cy + h), static_cast<float>(cx), static_cast<float>(cy), f.angulo);

        if (f.relleno) RellenarPoligono(v, 4, f.colorRelleno);
        for (int i = 0; i < 4; ++i)
            DibujarLinea(v[i].x, v[i].y, v[(i + 1) % 4].x, v[(i + 1) % 4].y, f.colorFigura);
    }
    else if (f.tipo == MODO_TRIANGULO)
    {
        float cx = (f.pts[0].x + f.pts[1].x + f.pts[2].x) / 3.0f;
        float cy = (f.pts[0].y + f.pts[1].y + f.pts[2].y) / 3.0f;

        Punto v[3];
        for (int i = 0; i < 3; ++i)
            v[i] = Rotar(static_cast<float>(f.pts[i].x), static_cast<float>(f.pts[i].y), cx, cy, f.angulo);

        if (f.relleno) RellenarPoligono(v, 3, f.colorRelleno);
        for (int i = 0; i < 3; ++i)
            DibujarLinea(v[i].x, v[i].y, v[(i + 1) % 3].x, v[(i + 1) % 3].y, f.colorFigura);
    }
    else
    {
        float cx = 0.0f;
        float cy = 0.0f;
        for (size_t i = 0; i < f.pts.size(); ++i)
        {
            cx += f.pts[i].x;
            cy += f.pts[i].y;
        }
        cx /= f.pts.size();
        cy /= f.pts.size();

        Punto anterior = Rotar(static_cast<float>(f.pts[0].x), static_cast<float>(f.pts[0].y), cx, cy, f.angulo);
        FijarColorCelda(anterior.x, anterior.y, f.colorFigura);

        for (size_t i = 1; i < f.pts.size(); ++i)
        {
            Punto actual = Rotar(static_cast<float>(f.pts[i].x), static_cast<float>(f.pts[i].y), cx, cy, f.angulo);
            DibujarLinea(anterior.x, anterior.y, actual.x, actual.y, f.colorFigura);
            anterior = actual;
        }
    }
}

void LimpiarGrilla()
{
    for (int y = 0; y < GRILLA_FILAS; ++y)
        for (int x = 0; x < GRILLA_COLUMNAS; ++x)
            FijarColorCelda(x, y, colorGrilla);
}

// Borra la grilla y dibuja todas las figuras
void Redibujar()
{
    LimpiarGrilla();

    for (size_t i = 0; i < figuras.size(); ++i)
        DibujarFigura(figuras[i]);

    for (size_t i = 0; i < clics.size(); ++i)
        FijarColorCelda(clics[i].x, clics[i].y, colorPunto);

    SubirGrillaGPU();
}

int ClicsNecesarios(Modo modo)
{
    if (modo == MODO_TRIANGULO) return 3;
    return 2;
}

void CambiarModo(GLFWwindow* window, Modo nuevoModo)
{
    modoActual = nuevoModo;
    clics.clear();
    dibujandoLibre = false;

    std::string titulo = std::string("Paint - ") + nombresModo[nuevoModo];
    glfwSetWindowTitle(window, titulo.c_str());

    Redibujar();
}

// Mouse (pixeles) a celda de la grilla
void ObtenerCelda(GLFWwindow* window, int& columna, int& fila)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int anchoVentana, altoVentana;
    glfwGetWindowSize(window, &anchoVentana, &altoVentana);

    if (anchoVentana == 0 || altoVentana == 0)
    {
        columna = 0;
        fila = 0;
        return;
    }

    columna = static_cast<int>(xpos / anchoVentana * GRILLA_COLUMNAS);
    fila = static_cast<int>((altoVentana - ypos) / altoVentana * GRILLA_FILAS);

    if (columna < 0) columna = 0;
    if (columna >= GRILLA_COLUMNAS) columna = GRILLA_COLUMNAS - 1;
    if (fila < 0) fila = 0;
    if (fila >= GRILLA_FILAS) fila = GRILLA_FILAS - 1;
}

// Gira la ultima figura
void GirarUltima(float delta)
{
    if (figuras.empty()) return;

    float a = figuras.back().angulo + delta;
    while (a >= 360.0f) a -= 360.0f;
    while (a < 0.0f) a += 360.0f;

    figuras.back().angulo = a;
    Redibujar();
}

// Callback del mouse
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_RELEASE)
    {
        dibujandoLibre = false;
        return;
    }

    if (action != GLFW_PRESS) return;

    // Ignora clics sobre el panel de ImGui
    if (ImGui::GetIO().WantCaptureMouse) return;

    int columna, fila;
    ObtenerCelda(window, columna, fila);

    if (modoActual == MODO_LIBRE)
    {
        Figura f = NuevaFigura(MODO_LIBRE);
        Punto p = { columna, fila };
        f.pts.push_back(p);
        figuras.push_back(f);
        dibujandoLibre = true;
    }
    else
    {
        Punto p = { columna, fila };
        clics.push_back(p);

        if (static_cast<int>(clics.size()) == ClicsNecesarios(modoActual))
        {
            Figura f = NuevaFigura(modoActual);
            f.pts = clics;
            figuras.push_back(f);
            clics.clear();
        }
    }

    Redibujar();
}

// Callback del teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_L) CambiarModo(window, MODO_LINEA);
        else if (key == GLFW_KEY_C) CambiarModo(window, MODO_CIRCULO);
        else if (key == GLFW_KEY_F) CambiarModo(window, MODO_LIBRE);
        else if (key == GLFW_KEY_T) CambiarModo(window, MODO_TRIANGULO);
        else if (key == GLFW_KEY_Q) CambiarModo(window, MODO_CUADRADO);
        else if (key == GLFW_KEY_R)
        {
            figuras.clear();
            clics.clear();
            Redibujar();
        }
        else if (key == GLFW_KEY_Z)
        {
            if (!figuras.empty()) figuras.pop_back();
            Redibujar();
        }
    }

    if (key == GLFW_KEY_LEFT) GirarUltima(5.0f);
    else if (key == GLFW_KEY_RIGHT) GirarUltima(-5.0f);
}

// Callback de cambio de tamano (la grilla se agranda)
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // 1. CONSTRUCT
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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader shader;

    // 2. GEOMETRY
    CrearCuadradosGrilla(verticesGrilla, indicesGrilla, GRILLA_COLUMNAS, GRILLA_FILAS, colorGrilla);

    // 3. BUFFERS
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

    // 4. CONTINUUM
    while (!glfwWindowShouldClose(window))
    {
        // Dibujo libre
        if (dibujandoLibre && !figuras.empty())
        {
            int columna, fila;
            ObtenerCelda(window, columna, fila);

            Figura& f = figuras.back();
            Punto ultimo = f.pts.back();

            if (columna != ultimo.x || fila != ultimo.y)
            {
                Punto nuevo = { columna, fila };
                f.pts.push_back(nuevo);
                DibujarLinea(ultimo.x, ultimo.y, columna, fila, f.colorFigura);
                SubirGrillaGPU();
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Panel de herramientas
        ImGui::Begin("Herramientas");

        ImGui::Text("Modo: %s", nombresModo[modoActual]);
        if (ImGui::Button("Linea")) CambiarModo(window, MODO_LINEA);
        ImGui::SameLine();
        if (ImGui::Button("Circulo")) CambiarModo(window, MODO_CIRCULO);
        ImGui::SameLine();
        if (ImGui::Button("Libre")) CambiarModo(window, MODO_LIBRE);
        if (ImGui::Button("Triangulo")) CambiarModo(window, MODO_TRIANGULO);
        ImGui::SameLine();
        if (ImGui::Button("Cuadrado")) CambiarModo(window, MODO_CUADRADO);

        ImGui::Separator();
        ImGui::ColorEdit4("Borde", colorFigura);
        ImGui::ColorEdit4("Relleno", colorRelleno);
        ImGui::Checkbox("Rellenar figuras", &rellenar);
        ImGui::ColorEdit3("Fondo", colorFondo);

        ImGui::Separator();
        ImGui::Text("Girar la ultima figura");
        if (!figuras.empty())
        {
            float angulo = figuras.back().angulo;
            if (ImGui::SliderFloat("Angulo", &angulo, 0.0f, 360.0f))
            {
                figuras.back().angulo = angulo;
                Redibujar();
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Deshacer"))
        {
            if (!figuras.empty()) figuras.pop_back();
            Redibujar();
        }
        ImGui::SameLine();
        if (ImGui::Button("Limpiar"))
        {
            figuras.clear();
            clics.clear();
            Redibujar();
        }

        ImGui::End();
        ImGui::Render();

        glClearColor(colorFondo[0], colorFondo[1], colorFondo[2], colorFondo[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Usar();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, cantidadIndices, GL_UNSIGNED_INT, 0);

        // ImGui encima de la grilla
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 5. CLEAN MEMORY
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &g_VBO);
    glDeleteBuffers(1, &EBO);
    shader.Borrar();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}