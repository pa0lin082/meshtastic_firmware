

#Firmware customization


Crea un nuovo modulo in src/modules

inizializzalo nella funzione setupModules in src/modules/Modules.cpp





# Build per heltec-v3

bin/build-esp32.sh heltec-v3


# Deploy per heltec-v3
sh release/device-update.sh -p /dev/tty.usbserial-0001 -f release/firmware-heltec-v3-99.7.6.67e3a17b2-update.bin




Lista delle seriali disponibili:

ls /dev/tty.*



Lettura da SERIALE:
screen  /dev/tty.usbserial-0001 115200  (per uscire ctrl+a -> k [o d per detach]) 




#Gestion bluethoot:

system_profiler SPBluetoothDataType
