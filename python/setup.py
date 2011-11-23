import sys, os
from distutils.core import setup, Extension

script_dir = os.path.dirname(os.path.realpath( __file__ ))
top_dir = os.path.dirname(script_dir)
inc_dir = os.path.join(top_dir, "include")
lib_dir = os.path.join(top_dir, "lib")

pycrm114_module = Extension('pycrm114',
                            sources = ['pycrm114_module.c'],
                            include_dirs = [inc_dir],
                            library_dirs = [lib_dir],
                            runtime_library_dirs = [lib_dir],
                            libraries = ['tre', 'crm114'],
                            extra_compile_args=['-Wall', '-g'])

setup(name = 'pycrm114',
      version = '0.1.0',
      description = 'Python interface to libcrm114',
      author = 'Prashanth Mundkur',
      author_email = 'prashanth.mundkur at gmail.com',
      url = 'https://github.com/pmundkur/libcrm114',
      license = 'LGPL',
      ext_modules = [pycrm114_module])
