#/bin/sh

WORK="../../dist/pakobots/pakobots_1.0.0"

mkdir -p $WORK
cp -r ./DEBIAN $WORK
cp -r ./usr $WORK
cp -r ../../dist/PakoBots/* $WORK/usr/share/pakobots

sudo chown -R root:root $WORK
sudo dpkg-deb --build $WORK
