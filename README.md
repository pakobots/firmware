# Pako Bot Flash GUI

This branch contains the code for the ESP32 flash GUI application.

## Lastest Releases
[Latest Release](https://github.com/pakobots/firmware/releases)

![OSI Certified](https://origami3.com/osi_keyhole_100X100_90ppi.png)

OSI Certified Open Source Software

## Installation
Ensure the latest version of Kivy is installed: https://kivy.org/docs/installation/installation.html

Execute 'sudo ./setup.py install' from the command line to install

## Creating the OSX installer
Install kivy according to "Homebrew" or "MacPorts with Pip" instructions: https://kivy.org/docs/installation/installation-osx.html

Ensure the latest version of pyinstaller is available: pip install pyinstaller --upgrade

Package the app:
```
pyinstaller -y --clean --windowed PakoBots.spec
pushd dist
hdiutil create ./PakoBots.dmg -srcfolder PakoBots.app -ov
popd
```

## Running application
After installation execute 'pakobots' on the command line
