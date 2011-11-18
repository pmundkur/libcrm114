import sys, os
from distutils.core import setup, Extension

pycrm114_module = Extension('pycrm114',
                            sources = ['pycrm114_module.c'],
                            include_dirs = ['../include'],
                            library_dirs = ['/home/mundkur/proj/libcrm114/lib'],
                            runtime_library_dirs = ['/home/mundkur/proj/libcrm114/lib'],
                            libraries = ['tre', 'crm114'],
                            extra_compile_args=['-Wall', '-g'])

setup(name = 'pycrm114',
      version = '0.1',
      description = 'Python interface to libcrm114',
      ext_modules = [pycrm114_module])
