## Arquitectura y Componentes Clave

1. **Capa de Presentación y Ventana (`GLFW` + `GLAD`):** Gestiona el contexto OpenGL 3.3, la captura de periféricos (mouse/teclado) y el mapeo de coordenadas de pantalla a índices discretos de la grilla.
2. **Interfaz Gráfica (`Dear ImGui`):** Renderiza el panel de control *"Herramientas"* en una capa superior (`overlay`), interceptando eventos de entrada mediante `WantCaptureMouse` y `WantCaptureKeyboard` para evitar conflictos con el lienzo.
3. **Motor de Rasterización (`CPU`):**
   * **Modelo de Datos:** No almacena un mapa de bits estático, sino un `std::vector<Figura>` con los puntos de control y propiedades de cada entidad, permitiendo edición no destructiva.
   * **Algoritmos:**
     * *Bresenham Line (`DibujarLineaH` / `DibujarLineaV`):* Selección de píxeles mediante aritmética entera y parámetro de decisión $p$, bifurcándose según la pendiente ($\vert{}\Delta x\vert{} > \vert{}\Delta y\vert{}$).
     * *Bresenham Midpoint Circle (`DibujarCirculo`):* Cálculo de un octante ($45^\circ$) con simetría de 8 vías.
     * *Scanline Polygon Fill (`RellenarPoligono`):* Intersección horizontal fila por fila ($Y_{min}$ a $Y_{max}$) para el relleno de polígonos rotados.
     * *Transformaciones 2D (`Rotar`):* Rotación euclidiana alrededor del centroide de la figura previa a la discretización.
4. **Capa de Renderizado (`Shader.h` + `OpenGL`):**
   * La grilla de $50 \times 50$ se genera una única vez como **2,500 quads** (10,000 vértices / 5,000 triángulos) en coordenadas normalizadas (`NDC` $[-1, 1]$).
   * Los shaders reciben posición (`location = 0`) y color (`location = 1`) por vértice, dibujando toda la matriz en una sola llamada (`glDrawElements`).

---

## Pipeline de Ejecución y Renderizado

```text
[ Entrada Usuario ] (Clics / Teclado / Sliders ImGui)
         │
         ▼
[ Conversión de Coordenadas ] (Píxeles Ventana 800x800 ──► Celda Grilla [0..49, 0..49])
         │
         ▼
[ Actualización de Escena (CPU) ]
   ├── LimpiarGrilla() ──► Restaura color base en el vector `verticesGrilla`
   ├── Transformación  ──► Aplica matriz de rotación 2D sobre los vértices de cada `Figura`
   └── Rasterización   ──► Ejecuta Bresenham / Scanline y actualiza atributos `color` en RAM
         │
         ▼
[ Transferencia CPU ──► GPU ] (`SubirGrillaGPU()`)
   └── glBufferSubData() actualiza únicamente el VBO dinámico (`GL_DYNAMIC_DRAW`)
         │
         ▼
[ Pipeline Gráfico OpenGL ]
   ├── Vertex Shader   ──► Proyecta coordenadas NDC y pasa `vColor`
   ├── Rasterizador HW ──► Ensambla los 5,000 triángulos indexados (`EBO`)
   └── Fragment Shader ──► Pinta cada celda en el Framebuffer
         │
         ▼
[ Renderizado UI ] (`ImGui_ImplOpenGL3_RenderDrawData`)
   └── Dibuja el panel de herramientas sobre el lienzo y hace `glfwSwapBuffers()`
