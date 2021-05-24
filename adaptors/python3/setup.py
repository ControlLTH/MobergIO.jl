#!/usr/bin/env python
from distutils.core import setup, Extension

module = Extension('moberg',
                   sources = ['python-moberg.c'],
                   include_dirs=['../..'],
                   libraries=['moberg'])
longdesc = '''This extension provides bindings to the Moberg I/O library
'''

try:
    import os
    VERSION=os.environ['MOBERG_VERSION']
except KeyError:
    VERSION='_UNKNOWN_'
    pass

setup(
    name        = 'python-moberg',
    version     = VERSION,
    description = 'Python bindings to the Moberg I/O library',
    long_description = longdesc,
    author      = 'Anders Blomdell',
    author_email = 'anders.blomdell@control.lth.se',
    url         = 'http://gitlab.control.lth.se/anders_blomdell/moberg.git',
    license     = 'GPLv2',
    platforms   = 'Linux',
    ext_modules = [module])
