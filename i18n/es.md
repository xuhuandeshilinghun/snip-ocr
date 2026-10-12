**Idioma del Readme / Readme Language：**
[English](../README.md)｜[日本語](ja.md)｜[한국어](ko.md)｜[Русский](ru.md)｜[Français](fr.md)｜[Deutsch](de.md)｜[Español](es.md)｜[Português](pt.md)｜[简体中文](zh.md)｜[繁體中文](tw.md)


💖 Apoya este proyecto: [Ko‑fi](https://ko‑fi.com/edenls)


# SnapOCR

> Utilidad de escritorio de alto rendimiento para capturas de pantalla, pegatinas de escritorio y reconocimiento OCR sin conexión, desarrollada en C++17 / Qt6 para la plataforma Windows.

SnapOCR es una utilidad de escritorio todo‑en‑uno de código abierto para Windows 10 / 11 x64. Integra capturas de pantalla de alto rendimiento, vista previa flotante de pegatinas de escritorio, anotación de imágenes, reconocimiento OCR multidioma sin conexión, capturas retardadas en bucle y capacidades de actualización automática.
**Idiomas admitidos en la interfaz: chino simplificado, chino tradicional, inglés, japonés, coreano, alemán, francés, ruso; el reconocimiento de texto OCR está disponible para todos los idiomas mencionados.**

Todo el procesamiento de imágenes y el reconocimiento de texto se ejecutan **localmente en tu dispositivo. Las capturas de pantalla nunca se suben a la nube, protegiendo tu privacidad.**

Adecuado para extraer material de oficina, asistencia de lectura y traducción, captura de texto de documentos web, captura de recursos para desarrolladores, anotación de capturas para demostraciones docentes y muchos otros escenarios.

Desarrollado con API nativas de Windows y Qt6. Es compatible con configuraciones de varios monitores de alto‑DPI, ofrece temas claro / oscuro, interfaz de usuario completamente internacionalizada multidioma y puede ejecutarse en segundo plano desde la bandeja del sistema.

---

## Características

### 📷 Módulo de captura de pantalla

- **Captura de pantalla con tecla de acceso global**: Implementada mediante el gancho de teclado de bajo nivel de Windows `WH_KEYBOARD_LL`. Activa la captura con una sola tecla independientemente del foco de la ventana activa.
- **Selección de región libre**: Arrastra el ratón para dibujar un rectángulo de selección con 8 puntos de control de redimensión. Ajuste fino con teclas de flecha: 1 píxel por paso, `Mayús + Tecla de flecha` para movimiento de 10 píxeles para una selección precisa por píxel.
- **Ajuste automático a bordes de ventana**: Inspirado en Snipaste. Los bordes de la selección se ajustan automáticamente a los bordes de las ventanas con un umbral predeterminado de 8 píxeles para capturar rápidamente ventanas completas.
- **Detección precisa de ventanas y controles**: Usa `WindowFromPoint` + `RealChildWindowFromPoint` para detectar controles secundarios como botones, cuadros de entrada y barras de herramientas para seleccionar directamente el área del control.
- **Obtención de límites reales de ventana mediante DWM**: Llama a `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` para excluir las sombras de ventana y obtener las dimensiones reales de renderizado de la ventana.
- **Lupa de píxeles**: Superposición de lupa en el modo de captura, muestra los valores de color RGB / HEX y las coordenadas de pantalla. Alterna el formato de color para la selección de color e inspección de píxeles.
- **Etiqueta de dimensión de selección**: Etiqueta de tamaño `W × H` con fondo azul redondeado que se muestra cerca de la selección. El color del texto se adapta automáticamente al tema claro / oscuro.
- **Barra de herramientas de captura**: Aparece una barra de herramientas inferior después de la captura de región. Admite fijar pegatinas, reconocimiento OCR, reconocimiento de código QR, copiar imagen, guardar como PNG y cancelar.

### 🖼️ Pegatinas de escritorio y anotación

- **Pegatinas flotantes de escritorio**: Fija las capturas realizadas directamente en pantalla en la posición original, similar a Snipaste. Compatibilidad total con la relación de píxeles del dispositivo DPR para evitar borros en pantallas de alta resolución.
- **Juego completo de herramientas de anotación**: Haz clic derecho para abrir la barra de herramientas de dibujo: seleccionar, bolígrafo, flecha, rectángulo, elipse, resaltado, pixelado de mosaico, nota de texto, borrador.
- **Transformación de imagen**: Las pegatinas admiten escalado del 20%‑500%; rotación de 90 grados en sentido horario / antihorario, volteo horizontal y vertical. Todas las operaciones trabajan sobre la resolución física original de la imagen sin pérdida de calidad.
- **Exportación con anotaciones integradas**: Fusiona todas las anotaciones dibujadas en la imagen de origen y exporta un PNG completo conservando la resolución original.
- **Panel de selección de color de pantalla**: Implementación independiente del selector de color, admite muestrear cualquier píxel de la pantalla, compatible con interfaz multidioma.
- **Alternancia de sombra de ventana de pegatina**: Habilita o deshabilita el efecto de sombra para las ventanas de pegatina para el estilo visual nativo de esquinas redondeadas de Windows 11.
- **Gestión de grupos de pegatinas**: Varios grupos de pegatinas (grupo predeterminado + grupos personalizados). Cambia de grupo para ocultar las pegatinas del grupo actual y restaurar la disposición de otro grupo.
- **Historial de capturas persistente**: Las capturas originales se guardan como PNG dentro de la carpeta local `history`. La posición, escala y estados de anotación de las pegatinas se almacenan en archivos `.sp1` con ofuscación ligera XOR+Base64 (no es cifrado fuerte, solo para prevenir modificaciones accidentales).
- **Restauración automática de pegatinas al reiniciar**: Cuando la aplicación se vuelve a ejecutar, todas las pegatinas flotantes restauran sus últimas posiciones, escalas y asignaciones de grupo.

### 🔍 Reconocimiento de texto OCR sin conexión

- **Carga paralela de varios modelos**: Carga varios modelos de reconocimiento simultáneamente. Admite texto mixto chino‑inglés, coreano, cirílico ruso y escrituras latinas.
- **Detección automática de idioma**: El algoritmo analiza las proporciones de caracteres en la imagen y cambia automáticamente al modelo OCR correspondiente, no requiere selección manual de idioma.
- **Inferencia CPU ONNX Runtime**: Se ejecuta completamente en la CPU. **No se requiere GPU discreta**, funciona en portátiles comunes.
- **Hilo de trabajo en segundo plano sin bloqueo**: Las tareas OCR se ejecutan dentro de un hilo de trabajo QThread independiente para que la interfaz de usuario permanezca responsiva durante el reconocimiento.
- **Diálogo de resultado de reconocimiento**: Ventana de salida OCR editable. Admite copia al portapapeles con un clic y guardar texto en archivo local.

### ⏱️ Captura retardada

- **Captura retardada y en bucle**: Establece retardo de 1‑10 segundos, configura el número de capturas en bucle para el guardado automático por lotes.
- **Notificación toast del sistema**: Aviso nativo de Windows cuando finaliza la secuencia de captura, muestra la ruta de guardado y la cantidad de capturas. Haz clic en la notificación para copiar la ruta del archivo.

### 🔄 Sistema de actualización automática

- **Comprobación de versión**: Comprobación automática de versión al iniciar; comprobación manual disponible dentro del panel de ajustes.
- **Mecanismo de actualización diferencial**: Genera paquetes de versión completa y paquetes de parche diferencial basados en la versión de referencia. Solo se descargan los archivos modificados para reducir el tamaño de descarga.
- **Enrutamiento de doble origen**: Origen de descarga de Gitee para usuarios de China continental, GitHub Releases para usuarios extranjeros para resolver diferencias de acceso a la red.
- **Binario de actualizador independiente**: `updater.exe` construido sobre API Win32 sin procesar + minizip, sin dependencias de Qt.
- **Preservar configuración y datos de usuario**: Realiza copia de seguridad del archivo de configuración de usuario `.config_settings.json` durante la actualización. Restaura las preferencias después de la actualización; se conservan las teclas de acceso rápido, los temas y la configuración de los grupos de pegatinas.
- **Flujo de reinicio tras actualización**: Una vez completada la actualización, invoca `ShellExecuteExW` para lanzar el nuevo binario del programa con el argumento `--restart` para saltarse el mutex de instancia única y realizar un reinicio limpio.

### ⚙️ Integración del sistema

- **Servicio en segundo plano en bandeja del sistema**: Minimiza a la bandeja. El menú contextual derecho de la bandeja incluye captura de pantalla, OCR, captura retardada, grupos de pegatinas, borrar historial, preferencias, reiniciar aplicación y salir.
- **Internacionalización de la aplicación**: Interfaz integrada de chino simplificado `zh_CN` e inglés `en_US`. Cambia el idioma en tiempo de ejecución. Los diálogos integrados de Qt también están localizados.
- **Adaptación de tema claro‑oscuro**: Cambio manual o según el sistema. Las máscaras de superposición, los separadores de menú y las sombras de ventana se adaptan en consecuencia.
- **Mutex de instancia única**: Mutex global evita lanzamientos duplicados de la aplicación. Un argumento especial salta la comprobación del mutex durante el procedimiento de reinicio tras actualización.
- **Compatibilidad con rutas UTF‑8 en chino**: Solución alternativa para un error nativo de la API Qt que puede devolver valores vacíos para rutas de Windows UTF‑8 que contienen caracteres chinos.

---

## Vista previa

| Región de captura | Anotación de pegatina | Resultado OCR |
| --- | --- | --- |
| ![captura](../docs/images/capture.png) | ![pegatina](../docs/images/sticker.png) | ![ocr](../docs/images/ocr.png) |

## Descarga e instalación

1. Obtén la última versión desde [Releases](https://github.com/xuhuandeshilinghun/snip%E2%80%91ocr/releases): `SnapOCR‑<version>‑win64.zip`
2. Extrae el archivo comprimido en cualquier carpeta local. Software portátil, no requiere instalador.
3. Descarga el archivo comprimido independiente del modelo OCR y descomprímelo en la subcarpeta `model/` del programa.
4. Ejecuta `SnapOCR.exe`.

> 
> **Requisito obligatorio del sistema: Windows 10 / Windows 11 x64**
> Los archivos del modelo OCR son grandes y no se incluyen dentro del archivo principal del programa.
> Evita extraer en directorios protegidos por el sistema como `C:\Windows` o `Program Files` para prevenir problemas de permisos de lectura‑escritura.

## Atajos de teclado

| Atajo | Función |
| --- | --- |
| `F1` | Activar modo de captura de pantalla |
| `Ctrl+Shift+S` | Capturar pantalla y ejecutar reconocimiento OCR |
| `Esc` | Salir / cancelar captura |
| `Rueda del ratón` | Ajustar selección en 1 píxel |
| `C` | Copiar valor de color del selector de la lupa |

## Pila tecnológica

- Lenguaje: C++17
- Marco GUI: Qt 6.5.3+
- Sistema de compilación: CMake 3.20+ / MSVC
- Inferencia OCR: ONNX Runtime 1.18.1 CPU
- Procesamiento de imágenes: OpenCV 4.x
- Análisis JSON: nlohmann/json
- Compresión‑descompresión Zip: minizip
- Interfaces de sistema Windows: API Win32, DWM, gancho de teclado de bajo nivel
- Compilador: MSVC2019 / MSVC2022

## Notas de privacidad y seguridad

1. El procesamiento de capturas, renderizado de pegatinas y reconocimiento OCR se ejecutan completamente de forma local. El software **nunca subirá automáticamente capturas ni texto reconocido a internet.**
2. La función de actualización automática se conecta a los servidores de GitHub / Gitee únicamente para la validación de versiones y la descarga de paquetes de actualización.
3. Las capturas originales se almacenan como PNG plano dentro de la carpeta `history`. Limpia manualmente la carpeta del historial si has capturado contraseñas, documentos de identidad u otro material sensible.
4. Los archivos `.sp1` e `index.dat` usan ofuscación simple XOR+base64 **que NO es cifrado seguro. No almacenes datos altamente confidenciales dentro de estos archivos.**
5. Puedes desactivar el guardado del historial de capturas dentro de los ajustes de la aplicación.

## Limitaciones conocidas

- Actualmente solo se admite Windows 10 / 11 x64. Linux y macOS no están disponibles.
- El OCR depende de la CPU; la velocidad de reconocimiento depende del rendimiento de tu CPU.
- Los archivos del modelo OCR son grandes y deben descargarse e implementarse manualmente.
- Las configuraciones de varios monitores con DPI mixto deben probarse en uso real; pueden aparecer problemas de compatibilidad en configuraciones de hardware especiales poco frecuentes.

## Estado de las funcionalidades

**Completadas**

- Captura de pantalla
- Pegatinas de escritorio
- Minimizar a bandeja del sistema
- Función OCR
- Reconocimiento de código QR
- Soporte multidioma (chino, inglés, japonés, coreano, alemán, francés, ruso)
- Captura en bucle retardada
- Funciones completas de la barra de herramientas

**No implementadas todavía**

- Captura de pantalla larga
- Compatibilidad con macOS

## Contribuciones

Se agradecen los informes de errores y las solicitudes de funcionalidad mediante Issues y Pull Requests.

1. Haz un fork de este repositorio
2. Crea rama de funcionalidad: `git checkout -b feature/your‑feature‑name`
3. Confirma tus cambios
4. Haz push de la rama y envía una PR

> 
> Notas para el envío: NO confirmes artefactos de compilación, contenidos del directorio build ni archivos de modelo grandes. Prefiere submódulos git para bibliotecas de terceros.

## Desarrollador: Compilación, empaquetado y publicación

> 
> No destinado a usuarios finales, reservado para los mantenedores del proyecto.

### Preprocesamiento de recursos antes de la publicación

**Debes** ejecutar estos scripts de compilación de recursos antes de cada empaquetado de versión:

```
# Compilar los recursos de traducción multidioma
python tools\compile_locales.py
# Empaquetar los recursos de imágenes
python tools\pack_img_res.py
```

### Lógica de selección del paquete de actualización (reglas del programa de actualización automática)

| Versión instalada localmente | Tipo de paquete a descargar |
| --- | --- |
| V1.0.1 (versión oficial anterior) | Paquete delta (el más ligero, algunos MB solamente) |
| V1.0.0 (versión de referencia) | Paquete completo (cálculo de diferencias respecto a la referencia 1.0.0) |
| Otras versiones antiguas | Paquete completo (cálculo de diferencias respecto a la referencia 1.0.0) |
| No instalado | **La versión de referencia V1.0.0 debe instalarse primero** |

> 
> ⚠️ `build_release_1.0.0` es el directorio de referencia global. **Généralo una sola vez, consérvalo definitivamente, no lo suprima ni lo modifique**. Todas las actualizaciones delta posteriores dependen de esta referencia.

### Ejemplos de script make_release.py

```
# 1.【Ejecutar una sola vez】Crear la versión de referencia 1.0.0
python tools/make_release.py --version 1.0.0 --no-build

# 2.Publicar la versión 1.0.1 (referencia fijada a 1.0.0, comparación con el antiguo build_release_1.0.0)
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --base-dir build_release_1.0.0 --no-build

# 3.Publicar la versión 1.0.2 (referencia fijada a 1.0.0, comparación con el antiguo build_release_1.0.1)
python tools/make_release.py --version 1.0.2 --make-update --old build_release_1.0.1 --base-dir build_release_1.0.0 --no-build

# 4.Publicar la versión 1.0.3 (referencia fijada a 1.0.0, comparación con el antiguo build_release_1.0.2)
python tools/make_release.py --version 1.0.3 --make-update --old build_release_1.0.2 --base-dir build_release_1.0.0 --no-build

# Comandos frecuentemente utilizados
# Flujo completo : compilación + recolección de artefactos de publicación
python tools/make_release.py --version 1.0.1

# Compilar + generar el paquete completo y el paquete delta de actualización
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0

# Binarios ya compilados, empaquetaje únicamente, ignorar etapa de compilación
python tools/make_release.py --version 1.0.1 --no-build

# Generar los paquetes de actualización con notas de versión y bandera de actualización forzada activada
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --update-log "1.Optimizar la interfaz de usuario\n2.Corregir los errores conocidos" --force-update
```

### Referencia sucinta de parámetros

- `--version x.x.x`: definir el número de versión objetivo de la publicación
- `--no-build`: ignorar la compilación C++, realizar únicamente el empaquetaje (usar cuando los binarios ya están construidos)
- `--make-update`: activar la generación del paquete delta ; produce a la vez el paquete completo y el parche delta
- `--old <dir>`: directorio de la versión oficial anterior, usado para la comparación de archivos a fin de generar el parche delta
- `--base-dir <dir>`: directorio de referencia global, siempre rellenar con `build_release_1.0.0`
- `--update-log`: notas de versión de esta versión ; usar `\n` para los saltos de línea
- `--force-update`: activar la bandera de actualización forzada ; el cliente impondrá la actualización

> 
> Los archivos generados por la compilación se encuentran en `build_release_<version>`. Sube los archivos ZIP de esta carpeta a GitHub Releases.

## Licencia

Licencia MIT, consulta el archivo `LICENSE` del directorio raíz.

## Apoya el proyecto

SnapOCR es totalmente de código abierto y gratuito para uso personal. Si encuentras esta herramienta útil, puedes apoyar el desarrollo continuo voluntariamente a través de Ko‑fi:
👉 [https://ko‑fi.com/edenls](https://ko%E2%80%91fi.com/edenls)

## Contacto y comentarios

‑ Correo electrónico: [senjin@foxmail.com](mailto:senjin@foxmail.com)
‑ Prefiere enviar informes de errores y sugerencias de funcionalidades a través de GitHub Issues.
