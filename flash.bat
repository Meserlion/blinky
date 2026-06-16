@echo off
REM Flash the built hex file to the board via avrdude.
REM Atmel Studio's $(OutDir)/$(TargetDir) macros don't resolve correctly
REM in this setup's External Tools config, so the path is hardcoded.
avrdude.exe -carduino -p m328p -P \\.\COM6 -b 57600 -U flash:w:"D:\Programs\test flash\flkash\flkash\Debug\flkash.hex":i
pause
