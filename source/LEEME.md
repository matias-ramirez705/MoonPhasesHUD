# Moon Phases HUD — Guía en español

Widget independiente para el HUD de Skyrim Special Edition que muestra las fases
actuales de las dos lunas de Nirn, **Masser y Secunda**, con la misma estética de la
insignia lunar de *By Time A Clock HUD*… pero como **mod propio**: sin reloj, sin
fechas y **sin depender del mod del reloj** (puedes usar ambos a la vez, o solo este).

Todos los textos —del menú de ajustes y del widget— se traducen editando archivos
`.txt` simples.

---

## Requisitos

- Skyrim Special Edition + [SKSE64](https://skse.silverlock.org/)
- Opcional (para el menú de ajustes dentro del juego): [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)
  (se abre con **F1** por defecto). Sin él, todo se configura igual desde
  `Data\SKSE\Plugins\MoonPhaseHUD.ini`.

## Instalación

1. Consigue `MoonPhaseHUD.dll` (compílalo una sola vez — ver más abajo — o descárgalo
   de una versión publicada).
2. Con MO2: instala el ZIP como cualquier mod. A mano: copia el contenido de `Data\`:
   - `Data\SKSE\Plugins\MoonPhaseHUD.dll` y `MoonPhaseHUD.ini`
   - `Data\Interface\MoonPhaseHUD.swf`
   - `Data\Interface\Translations\MoonPhaseHUD_english.txt`, `MoonPhaseHUD_spanish.txt`
     (y `MoonPhaseHUD_template.txt` si quieres traducir a otro idioma)
3. El widget aparece junto al HUD. Los ajustes están en el menú del SKSE Menu
   Framework (F1), sección **Moon Phases HUD**.

## El widget

- Insignia de cielo nocturno con estrellas y las dos lunas dibujadas en su fase real:
  Masser completa su ciclo en **24 días** de juego y Secunda en **31** (los mismos
  valores que usa el mod del reloj, así que ambos siempre coinciden).
- Texto opcional con el nombre de la fase (**Luna llena**, **Creciente**, …), de
  Masser o de Secunda, a elegir.

## Menú de ajustes (F1 → Moon Phases HUD)

| Sección | Qué controla |
|---|---|
| **Comportamiento** | Activar, modo de visualización (siempre / por tecla / conmutar / periódico), tecla, ocultar en combate, diálogos y menús de mobiliario. |
| **Lunas** | Mostrar Masser y/o Secunda, mover cada una (X/Y), cambiar su tamaño, placa de cielo, estrellas, color personalizado de la insignia. |
| **Texto de fase** | Mostrar u ocultar el nombre de la fase, de qué luna, posición (X/Y), tamaño, sombra y color del texto. |
| **Posición** | Anclaje en pantalla (9 posiciones), desplazamientos, tamaño y opacidad del widget completo. |
| **Idioma** | Idioma activo, archivo de traducción cargado, botón **Recargar traducción** e instrucciones. |
| **Ahora mismo** | Vista previa: fase actual de Masser y de Secunda. |

Los cambios se aplican al instante y se guardan al cerrar el menú (o con *Guardar ahora*).

## Traducción — cómo funciona

1. **Detección automática**: el mod lee el idioma del juego
   (`SkyrimPrefs.ini`, ajuste `sLanguage`). Con el juego en español carga
   `MoonPhaseHUD_spanish.txt` automáticamente.
2. **Forzar un idioma**: escribe p. ej. `Language=spanish` en
   `Data\SKSE\Plugins\MoonPhaseHUD.ini` (vacío = automático).
3. **Editar textos**: abre
   `Data\Interface\Translations\MoonPhaseHUD_spanish.txt` y cambia lo que quieras.
   Formato: `clave = valor`, una por línea, UTF-8; `#` para comentarios y `\n` para
   saltos de línea (en los tooltips).
4. **Añadir un idioma nuevo**: copia `MoonPhaseHUD_template.txt` como
   `MoonPhaseHUD_<idioma>.txt` (p. ej. `MoonPhaseHUD_french.txt`) y traduce los
   valores. Ese único archivo cubre el mod entero: menú y widget.
5. **Claves que falten** → se muestran en inglés (el `MoonPhaseHUD_english.txt`,
   que además está compilado dentro del DLL como red de seguridad).
6. **Botón Recargar** (sección Idioma del menú): vuelve a leer el txt al momento,
   sin reiniciar el juego. Ideal mientras traduces.

> El nombre de las fases también llega traducido al SWF: si un día quieres otro
> término (p. ej. «Cuarta creciente» en vez de «Cuarto creciente»), edítalo en el
> txt y listo — no hace falta recompilar nada.

## Compilar el DLL (una sola vez)

El DLL necesita MSVC (Windows); dos caminos:

- **GitHub Actions (recomendado)**: sube la carpeta `source\` a un repositorio de
  GitHub. El flujo de trabajo incluido (`.github/workflows/build.yml`) compila solo y
  produce `MoonPhasesHUD.zip` con DLL + SWF + INI + traducciones, listo para instalar.
- **Local**: Visual Studio 2022 + CommonLibSSE-NG (rama `ng`) + vcpkg (versión fijada
  en `vcpkg.json`); luego `cmake --preset release` y `cmake --build build/release`.

El SWF ya viene compilado (`dist\Data\Interface\MoonPhaseHUD.swf`); solo el DLL
necesita este paso.

## Solución de problemas

| Problema | Solución |
|---|---|
| No aparece el widget | Falta `Data\Interface\MoonPhaseHUD.swf` o el DLL no se cargó (revisa `Data\SKSE\Plugins\MoonPhaseHUD.log` y que SKSE esté instalado). |
| No veo el menú de ajustes | Instala SKSE Menu Framework; mientras tanto usa el INI. |
| Sale texto en inglés | No se encontró el txt del idioma: comprueba el nombre (`MoonPhaseHUD_spanish.txt`) y la carpeta `Data\Interface\Translations\`, o fuerza `Language=` en el INI. |
| Cambié el txt y no se nota | Pulsa **Recargar traducción** en el menú (o reinicia el juego). |
| Se solapa con el reloj de By Time A Clock HUD | Este widget usa su propia capa (profundidad 14103 frente a 14100 del reloj) pero conviene darle otra posición: sección **Posición** del menú. |

## Créditos

- Derivado del código fuente abierto de **By Time A Clock HUD** de QY (Nexus 195093):
  arquitectura del widget, estilo de dibujo y modelo de fases lunares.
- Menú de ajustes gracias a **SKSE Menu Framework** (Nexus 120352).
