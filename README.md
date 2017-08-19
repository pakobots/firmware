# Pako Bot Firmware 

## ESP32

### Build Environment
Please refer to the documentation provided on the Espressif website for setting up the ESP32 build toolchain [Toolchain](https://esp-idf.readthedocs.io/en/v2.0/linux-setup.html)

### Build Commands

```
make menuconfig
make build
```

With the development board connected, the make file is able to upload and monitor the built
```
make flash
```

More information is provided on the Espressif website in regards to make commands and the monitoring feature [Monitoring Feature](http://esp-idf.readthedocs.io/en/latest/get-started/idf-monitor.html)

### Folder Structure
The most interesting code parts are found in the components directory. The components are arranged according to function of the system. The main.c file, found in the 'main' folder, is only a thin layer to launch the system. The robot component provides GPIO calls to manipulate movement and the LED functionality.

 
