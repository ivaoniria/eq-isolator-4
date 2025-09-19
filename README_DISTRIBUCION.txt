EQIsolator4 - VST3 listo para compartir (sin instalar Visual C++)

Como compilar el VST3 independiente:
1) En Windows, ejecuta el archivo: static_build.bat (doble click)
2) Espera a que termine la compilación (Release)
3) El archivo final se copia a la carpeta: dist\EQIsolator4.vst3

Como instalar el VST3 (usuario final):
1) Copia la carpeta/bundle EQIsolator4.vst3 dentro de:
   C:\Program Files\Common Files\VST3
2) Abre tu DAW (Ableton, Cubase, FL, etc.) y reescanea plugins VST3

Notas:
- Este VST3 no requiere JUCE ni Visual C++ Redistributable
- Es 64 bits y compatible con DAWs que soporten VST3
- Si Windows marca el archivo como bloqueado (Propiedades), marca "Desbloquear"
