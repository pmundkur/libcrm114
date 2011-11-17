import sys, os
from distutils.core import setup, Extension

pycrm114_module = Extension('pycrm114',
                             sources = ['pycrm114_module.c'],
                             include_dirs = ['../include'],
                             libraries = ['tre'],
                             extra_compile_args=['-Wall'])

setup(name = 'pycrm114',
      version = '0.1',
      description = 'Python interface to libcrm114',
      ext_modules = [pycrm114_module])
