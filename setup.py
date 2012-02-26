from distutils.core import setup, Extension

module1 = Extension('lsd',
                    sources = ['lsdPyAPI.c', 'lsd.c'],
                    include_dirs = ['/home/alex/tv\ proj/lsd/'],
                    libraries = ['m'],
                    library_dirs = ['/home/alex/tv\ proj/lsd/'])

setup (name = 'lsd',
       version = '1.5',
       description = 'API for using LSD library in Python',
       ext_modules = [module1])
