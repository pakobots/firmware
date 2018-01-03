#!/usr/bin/python

import sys
from setuptools import setup
from codecs import open
from os import path

here = path.abspath(path.dirname(sys.argv[0]))
with open(path.join(here, 'README.rst'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='pakobots',
    version='1.0.3',
    description='Pako Bots Flasher',
    long_description=long_description,
    url='http://invent.pakobots.com',
    author='Pako Bots a division of Origami 3',
    author_email='tech@origami3.com',
    license='GPL',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Robot Enthusiests',
        'Topic :: Software Development :: esp32 flash tool',
        'License :: OSI Approved :: GPL License',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
    ],
    packages=['pakobots'],
    keywords='robot esp32 flash programming pako bots',
    install_requires=['requests', 'esptool', 'kivy', 'pyserial'],
    entry_points={
        'console_scripts': [
            'pakobots=pakobots:main',
        ],
    },
)
