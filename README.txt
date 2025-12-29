VFO Si5351 v5.2 de Jan Ciger - Adaptado para Arduino Nano/Uno a 16 MHz

Este sketch está listo para usar en cualquier Arduino Nano o Uno estándar (cristal externo de 16 MHz).

Cambios realizados respecto al original (diseñado para 8 MHz interno):
- Si5351 inicializado correctamente para 16 MHz (sin parámetro extra, la librería Etherkit lo detecta automáticamente).
- Frecuencia inicial: 7.150.000 Hz
- Rango de frecuencia: 300 kHz a 30 MHz (ideal para probar filtros IF de 455/500 kHz más adelante)

Requisitos:
- Arduino IDE 1.8 o superior
- Bibliotecas (instalar desde el Gestor de Bibliotecas):
  → LiquidCrystal (viene incluida)
  → Bounce2 (by Thomas O. Fredericks)
  → Rotary (by Brian Low - la versión "Rotary-master" del ZIP de GitHub)
  → Etherkit Si5351 (by Jason Milldrum / NT7S)

Uso:
1. Abre la carpeta con el Arduino IDE (o el archivo .ino principal).
2. Selecciona placa: Arduino Nano (o Uno) y procesador ATmega328P (Old Bootloader si es Nano chino).
3. Compila y sube.
4. La primera vez (o si cambias valores en definitions.h), borra la EEPROM con el sketch "Borrar_EEPROM.ino" incluido para que arranque limpio en 7.150 MHz.

¡Disfruten del VFO! Calibren el Si5351 desde el menú para máxima precisión.

73
JuanCarlos