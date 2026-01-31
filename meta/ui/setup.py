from setuptools import setup, Extension
import pybind11

# To build 
# python setup.py build_ext --inplace

ext_modules = [
    Extension(
        'engine',
        ['../../src/engine_module.cpp'],
        include_dirs=[pybind11.get_include()],
        language='c++',
        extra_compile_args=['/std:c++20'],
    ),
]

setup(
    name='engine',
    ext_modules=ext_modules,
    zip_safe=False,
)