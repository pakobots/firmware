# -*- mode: python -*-

import os,platform

binaryDependencies=[]
binaryExcludes=[]

if platform.system() == 'Windows':
  from kivy.deps import sdl2, glew
  binaryDependencies = (sdl2.dep_bins + glew.dep_bins)
if platform.system() == 'Linux':
  from requests.packages import urllib3
if platform.system() == 'Darwin':
  binaryExcludes = ['_tkinter', 'Tkinter', 'enchant', 'twisted']

block_cipher = None

a = Analysis(['pakobots/__main__.py'],
             pathex=['C:\\Users\\maumock\\Desktop\\firmware'],
             binaries=[],
             datas=[('esptool/esptool.py','esptool'),('esptool/LICENSE','esptool'),('esptool/README.md','esptool'),('img/*', 'img'),('LICENSE','.'),('README.md', '.')],
             hiddenimports=[],
             hookspath=[],
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher)

pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)

exe = EXE(pyz,
          a.scripts,
          exclude_binaries=True,
          name='PakoBots',
          debug=False,
          strip=False,
          upx=True,
          console=False , icon='img\\logo.ico')

coll = COLLECT(exe,
               a.binaries,
               a.zipfiles,
               a.datas,
               *[Tree(p) for p in binaryDependencies],
               strip=None,
               upx=True,
               name='PakoBots')

app = BUNDLE(coll,
            name='PakoBots.app',
            icon=None,
            bundle_identifier='com.PakoBots')
