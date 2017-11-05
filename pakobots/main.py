#
# This file is part of Pako Bots division of Origami 3 Firmware.
#
#  Pako Bots division of Origami 3 Firmware is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Pako Bots division of Origami 3 Firmware is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Pako Bots division of Origami 3 Firmware.  If not, see <http://www.gnu.org/licenses/>.
#

import kivy
import sys
import glob
import os
import time
import threading
import serial
import zlib
import hashlib
import requests
import json

# Used in esptool. Added to enable pyinstaller to pull correct deps
import argparse
import inspect
import struct
import base64
import shlex
import copy
import io
# Used in esptool. Added to enable pyinstaller to pull correct deps

from distutils.version import StrictVersion

import esptool

kivy.require('1.10.0')  # replace with your current kivy version !
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.button import Button
from kivy.uix.image import Image
from kivy.graphics import Color, Rectangle
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.anchorlayout import AnchorLayout
from kivy.uix.gridlayout import GridLayout
from kivy.uix.progressbar import ProgressBar

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

class Release():
    bootloader = ''
    partitions = ''
    robot = ''
    version = ''

    def __init__(self, releaseDict):
        assets = releaseDict['assets']
        self.version = releaseDict['name']
        self.bootloader = [
            asset['browser_download_url'] for asset in assets if asset['name'].startswith('bootloader')][0]
        self.partitions = [
            asset['browser_download_url'] for asset in assets if asset['name'].startswith('partition')][0]
        self.robot = [
            asset['browser_download_url'] for asset in assets if asset['name'].startswith('robot')][0]


class Driver():
    stopped = threading.Event()
    window = {}

    def start(self, window):
        print('starting driver')
        threading.Thread(target=self.loadFirmware, args=(window,)).start()

    def stop(self):
        self.stopped.set()

    def loadFirmware(self, window):
        self.window = window
        self.window.setLoading(True)
        try:
            bots = self.findBots()
            if len(bots) == 0:
                self.window.setMessage(
                    'No robots found. Please ensure the robot is connected')
                self.window.setLoading(False)
                return
            if self.stopped.is_set():
                return
            print('getting latest release')
            release = self.getLatestRelease()
            time.sleep(1)
            if self.stopped.is_set():
                return
            print('release ready');
            self.load(bots[0]['chip'], release)
            time.sleep(3)
            self.window.setMessage('Robot firmware loaded successfully!')
            self.window.setLoading(False)
        except:
            self.window.setMessage('Error loading firmware. Please try again.')
            self.window.setLoading(False)

    def findBots(self):
        self.window.setMessage('Scanning for USB ports')
        self.window.setPercent(0)
        ports = self.getSerialPorts()
        out = []
        for i, p in enumerate(ports):
            # If a robot is found just exit for today.
            # In the future all robots will be caputred and programmed
            if len(out) > 0:
                return out
            self.window.setMessage('Looking for robot on USB port %s' % (p))
            self.window.setPercent((i + 1) / float(len(ports)) * 100)
            try:
                chip = esptool.ESPLoader.detect_chip(p)
                out.append({
                    'chip': chip,
                    'port': p
                });
            except esptool.FatalError:
                print
        return out

    def getSerialPorts(self):
        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
            except (OSError, serial.SerialException):
                pass
        return result

    def getLatestRelease(self):
        self.window.setMessage('Retrieving latest release from GitHub')
        self.window.setPercent(0)
        r = requests.get(
            'https://api.github.com/repos/pakobots/firmware/releases');
        esp32 = [release for release in r.json() if release['tag_name'].startswith('esp32')]
        latest = {'pako_version': 0}
        for x, release in enumerate(esp32):
            self.window.setPercent((x + 1) / float(len(esp32)) * 100)

            num = 0
            for i, dot in enumerate(reversed(release['tag_name'][6:].split('.', 3))):
                num += int(dot) << (i * 8)
            esp32[x]['pako_version'] = num

            if num > latest['pako_version']:
                latest = esp32[x]
        return Release(latest)

    def load(self, chip, release):
        self.window.setMessage('Loading firmware')
        self.window.setPercent(0)
        partitions = requests.get(release.partitions, stream=True).raw
        bootloader = requests.get(release.bootloader, stream=True).raw
        robot = requests.get(release.robot, stream=True).raw

        stub = chip.run_stub()
        flash_id = stub.flash_id()
        size_id = flash_id >> 16
        flash_size = esptool.DETECTED_FLASH_SIZES.get(size_id)

        stub.change_baud(921600)

        self.flash('bootloader', stub, bootloader, 0x1000)
        self.flash('partitions', stub, partitions, 0x8000)
        self.flash('robot', stub, robot, 0x10000)

        stub.hard_reset()

    def flash(self, partitionName, stub, file, address):
        if self.stopped.is_set():
            return
        self.window.setMessage('Reading remote firmware: %s' % (partitionName))
        self.window.setPercent(0)
        image = esptool.pad_to(file.read(), 4)
        calcmd5 = hashlib.md5(image).hexdigest()
        uncsize = len(image)
        uncimage = image
        image = zlib.compress(uncimage, 9)
        ratio = uncsize / len(image)
        blocks = stub.flash_defl_begin(uncsize, len(image), address)

        seq = 0
        written = 0
        t = time.time()
        stub._port.timeout = min(esptool.DEFAULT_TIMEOUT * ratio,
                                 esptool.CHIP_ERASE_TIMEOUT * 2)

        self.window.setMessage('Loading firmware: %s' % (partitionName))
        while len(image) > 0:
            if self.stopped.is_set():
                return
            self.window.setPercent(100 * (seq + 1) / blocks)
            print('\rWriting at 0x%08x... (%d %%)' % (address + seq *
                                                      stub.FLASH_WRITE_SIZE, 100 * (seq + 1) // blocks),)
            sys.stdout.flush()
            block = image[0:stub.FLASH_WRITE_SIZE]
            stub.flash_defl_block(block, seq)
            image = image[stub.FLASH_WRITE_SIZE:]
            seq += 1
            written += len(block)

        t = time.time() - t
        speed_msg = " (effective %.1f kbit/s)" % (uncsize / t * 8 / 1000)
        self.window.setMessage(
            'Loaded firmware, %s, in %.1f seconds' % (partitionName, t))
        print('\rWrote %d bytes (%d compressed) at 0x%08x in %.1f seconds%s...' % (
            uncsize, written, address, t, speed_msg))
        res = stub.flash_md5sum(address, uncsize)
        if res != calcmd5:
            print('File  md5: %s' % calcmd5)
            print('Flash md5: %s' % res)
            print('MD5 of 0xFF is %s' %
                  (hashlib.md5(b'\xFF' * uncsize).hexdigest()))
            raise FatalError("MD5 of file does not match data in flash!")


class RootWidget(GridLayout):

    percent = ProgressBar(value=0, max=100)
    message = Label(text='', markup=True)
    refresh = Button(background_normal=SCRIPT_DIR + '/img/refresh.png', background_down=SCRIPT_DIR + '/img/refresh_over.png', size_hint_x=None,
                     width=136, size_hint_y=None, height=35)
    loadingPanel = AnchorLayout()

    def __init__(self, **kwargs):
        super(RootWidget, self).__init__(**kwargs)

        self.add_widget(self.buildHeader())
        self.add_widget(self.buildPage())
        self.setMessage('Loading application')

    def setMessage(self, message):
        self.message.text = '[color=333333]%s[/color]' % (message)

    def setPercent(self, percent):
        self.percent.value = percent

    def setLoading(self, isLoading):
        self.loadingPanel.remove_widget(self.percent)
        self.loadingPanel.remove_widget(self.refresh)
        if isLoading:
            self.loadingPanel.add_widget(self.percent)
        else:
            self.loadingPanel.add_widget(self.refresh)

    def buildHeader(self):
        header = AnchorLayout(anchor_x='left', anchor_y='center',
                              size_hint_y=None, height=60, size_hint_x=None)
        image = Image(source=SCRIPT_DIR + '/img/logo.png', allow_stretch=True, keep_ratio=True,
                      size_hint_y=None, height=60, size_hint_x=None, width=163)
        header.add_widget(image)
        return header

    def buildPage(self):
        page = AnchorLayout()
        layout = GridLayout(cols=1, row_force_default=True, row_default_height=50,
                            size_hint_y=None, size_hint_x=None, width=500)
        layout.add_widget(self.message)
        layout.add_widget(self.loadingPanel)
        page.add_widget(layout)
        return page


class MainApp(App):
    driver = Driver()
    window = RootWidget(cols=1)

    def build(self):
        self.title = 'Pako Bots'
        self.root = self.window
        self.window.bind(size=self._update_rect, pos=self._update_rect)

        with self.window.canvas.before:
            # green; colors range from 0-1 not 0-255
            Color(0.95, 0.95, 0.95, 1)
            self.rect = Rectangle(size=self.window.size, pos=self.window.pos)
        self.window.refresh.bind(on_press=self.refresh)
        self.driver.start(self.window)
        return self.window

    def refresh(self, instance):
        self.driver.start(self.window)

    def on_stop(self):
        self.driver.stopped.set()

    def _update_rect(self, instance, value):
        self.rect.pos = instance.pos
        self.rect.size = instance.size


if __name__ == '__main__':
    MainApp().run()
