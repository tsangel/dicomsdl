import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

USE_DEBUG_MESSAGE=False

def get_dicomsdl_version():
    lines = open('src/include/dicomcfg.h').readlines()
    line = [l for l in lines if 'DICOMSDL_VERSION' in l][0]
    # line = 'const char *const DICOMSDL_VERSION = "0.105.1";\n'
    verstr = line.split('=')[-1].split('"')[1].strip()
    # verstr = '0.105.1'
    return verstr

def get_long_description():
    return open('README.md').read()

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        if USE_DEBUG_MESSAGE:
            cmake_args.append('-DUSE_DEBUG_MESSAGE=ON')

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)


classifiers = """
Development Status :: 3 - Alpha
Development Status :: 4 - Beta
Intended Audience :: Developers
Intended Audience :: Healthcare Industry
Intended Audience :: Science/Research
License :: OSI Approved :: MIT License
Operating System :: MacOS :: MacOS X
Operating System :: Microsoft :: Windows
Operating System :: POSIX :: Linux
Programming Language :: Python :: 2.7
Programming Language :: Python :: 3.5
Programming Language :: Python :: 3.6
Programming Language :: Python :: 3.7
Programming Language :: Python :: 3.8
Programming Language :: Python :: 3.9
Programming Language :: Python :: 3.10
Topic :: Scientific/Engineering
Topic :: Scientific/Engineering :: Medical Science Apps.
""".strip().splitlines()
classifiers = [l.strip() for l in classifiers]

setup(
    name='dicomsdl',
    version=get_dicomsdl_version(),
    author='Kim, Tae-Sung',
    author_email='taesung.angel@gmail.com',
    description='A fast and light-weighted DICOM software development library',
    long_description=get_long_description(),
    long_description_content_type='text/markdown',
    url='https://github.com/tsangel/dicomsdl',
    packages=find_packages('src/python'),
    package_dir={"dicomsdl":"src/python/dicomsdl"},
    ext_modules=[CMakeExtension('dicomsdl.dicomsdl')],
    entry_points={
        "console_scripts" : [
            "dicomdump=dicomsdl.dump:main",
            "dicomshow=dicomsdl.show:main",
        ]},
    classifiers=classifiers,
    python_requires='>=2.7, !=3.0.*, !=3.1.*, !=3.2.*, !=3.3.*, !=3.4.*, <4',
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
