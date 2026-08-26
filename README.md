# CAD+

**CAD+** es una aplicación de Diseño Asistido por Computadora (CAD) desarrollada en **C++20** utilizando la librería gráfica **SFML**. 

El proyecto está diseñado con una arquitectura modular (MVC) y se centra en la precisión matemática, la línea de comandos estilo AutoCAD y una interfaz gráfica profesional.

## 🚀 Características Actuales

- **Línea de Comandos:** Entrada de comandos por teclado con historial y scroll.
- **Sistemas de Coordenadas:** Soporte para coordenadas Absolutas (`x,y`), Relativas (`@dx,dy`) y Polares (`@dist<ang`).
- **Comandos de Dibujo:** Línea (`L`) y Círculo (`C`).
- **Navegación 2D:** Zoom centrado en el cursor y Pan (desplazamiento) con clic derecho.
- **Precisión (Object Snaps):** Forzado magnético a puntos finales de entidades existentes.
- **Interfaz Profesional:** Barra de herramientas con iconos, ventana de comandos con historial, barra de estado en tiempo real y ejes cartesianos configurables.
- **Arquitectura Limpia:** Separación estricta entre el motor lógico (`Engine`), el modelo de datos (`Document`) y la vista (`App`).

## 🛠️ Stack Tecnológico

- **Lenguaje:** C++20
- **Gráficos:** SFML 2.6
- **Gestor de Entorno/Build:** Pixi
- **Sistema de Build:** CMake

-----

## Instala las dependencias y compila:

```bash
   pixi install
   pixi run dev
```

*(Nota: Asegúrate de tener una fuente `.ttf` como `arial.ttf` o `DejaVuSans.ttf` en la carpeta `assets/` para que se renderice el texto correctamente).*

## Controles y Uso

### Ratón

| Acción                         | Resultado                                                     |
| ------------------------------ | ------------------------------------------------------------- |
| **Clic Izquierdo (Canvas)**    | Inyecta coordenadas / Confirma punto / Selecciona herramienta |
| **Clic Derecho (Arrastrar)**   | Pan (Mover la vista)                                          |
| **Rueda del Ratón (Canvas)**   | Zoom (Acercar / Alejar)                                       |
| **Rueda del Ratón (Comandos)** | Scroll del historial de comandos                              |

### Teclado (Línea de Comandos)

| Comando         | Descripción                             |
| --------------- | --------------------------------------- |
| `L` o `LINEA`   | Inicia el comando de dibujo de Línea    |
| `C` o `CIRCULO` | Inicia el comando de dibujo de Círculo  |
| `Z` o `BORRAR`  | Borra todas las entidades del dibujo    |
| `@dx,dy`        | Coordenada relativa (ej. `@100,0`)      |
| `@dist<ang`     | Coordenada polar (ej. `@50<45`)         |
| `Enter`         | Ejecutar comando / Confirmar coordenada |
| `Esc`           | Cancelar comando actual                 |

## 📂 Estructura del Proyecto

```textile
cad_cpp/
├── CMakeLists.txt      # Configuración de CMake
├── pixi.toml           # Configuración del entorno Pixi
├── README.md           # Documentación del proyecto
├── assets/             # Recursos (fuentes, iconos)
├── include/            # Encabezados (.hpp)
│   ├── app.hpp         # Interfaz gráfica y eventos (SFML)
│   ├── engine.hpp      # Lógica de comandos y estado
│   ├── document.hpp    # Modelo de datos (Entidades)
│   └── geometry.hpp    # Matemáticas (Puntos, Vectores)
── src/                # Implementación (.cpp)
    ├── main.cpp        # Punto de entrada
    ├── app.cpp         # Renderizado y gestión de eventos
    └── engine.cpp      # Procesamiento de comandos y coordenadas
```

## Hoja de Ruta (Roadmap)

- Comando Arco (`A`) y Polilínea (`PL`)
- Sistema de Undo / Redo (Ctrl+Z / Ctrl+Y)
- Selección y modificación de entidades existentes (Mover, Borrar, Copiar)
- Sistema de Capas (Layers)
- Guardar y Cargar archivos (DXF/JSON)

## 📄 Licencia

Este proyecto es de código abierto y está disponible bajo la licencia MIT.
