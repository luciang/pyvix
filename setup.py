#!/usr/bin/env python

# pyvix - Setup Script
# Copyright 2006 David S. Rushby
# Available under the MIT license (see docs/license.txt for details).

import os, os.path, re, sys, time

PLATFORM_IS_WINDOWS = sys.platform.lower().startswith('win')
if PLATFORM_IS_WINDOWS:
    import win32api

import distutils.core as dc

# Whether assertions should be enabled:
CHECKED_BUILD = 1

# Retrive the version number from a central text file for the sake of
# maintainability:
try:
    pyvixVersion = open('version.txt').read().strip()
except IOError:
    raise IOError(" File 'version.txt' is missing; cannot determine version.")

# Update the timestamp in pyvix's vix.py to reflect the time (UTC zone)
# when this build script was run:
vixModule = file('vix.py', 'rb')
try:
    vixModuleCode = vixModule.read()
finally:
    vixModule.close()

reTimestamp = re.compile(r"^(__timestamp__\s+=\s+')(.*?)(')$", re.MULTILINE)
vixModuleCode = reTimestamp.sub(
    r'\g<1>%s\g<3>' % time.strftime('%Y.%m.%d.%H.%M.%S.UTC', time.gmtime()),
    vixModuleCode
  )

vixModule = file('vix.py', 'wb')
try:
    vixModule.write(vixModuleCode)
finally:
    vixModule.close()


pythonModules = [
    'pyvix.__init__',
    'pyvix._support',
    'pyvix.vix',
  ]

dataFiles = []


def slurpDataPathsInSubDir(dirName):
    # Add an entry to dataFiles that'll cause the tests to be included in the
    # source distribution:
    paths = []
    for root, dirs, files in os.walk(os.path.join(os.curdir, dirName)):
        files = [
            f for f in files
            if not (f.startswith('.') or f.endswith('.pyc') or f.endswith('.pyo'))
          ]
        if 'CVS' in dirs:
            dirs.remove('CVS')

        useRoot = root
        ignorePrefix = os.curdir + os.sep
        if root.startswith(ignorePrefix):
            useRoot = root[len(ignorePrefix):]
        paths.extend([os.path.join(useRoot, f) for f in files])

    # distutils wants forward slashes in this context:
    paths = [p.replace(os.sep, '/') for p in paths]

    return paths

def formulateDataDir(dirName):
    return (
        ((PLATFORM_IS_WINDOWS and 'Lib') or 'lib/python'+sys.version[:3])
        + '/site-packages/pyvix/' + dirName
      )

for dataSubDir in ('docs', 'tests'):
    dataFiles.append(
        (
          # The first entry in this tuple will be interpreted by distutils as being
          # relative to sys.exec_prefix.
          formulateDataDir(dataSubDir),
          slurpDataPathsInSubDir(dataSubDir)
        )
      )


extensionModules = []

includeDirs = []
libNames = []
libDirs = []
macroDefs = []
extraCompilerArgs = []
extraLinkerArgs = []

if PLATFORM_IS_WINDOWS:
    def findAndLoadVixLocations():
        import _winreg
        r = _winreg.ConnectRegistry(None, _winreg.HKEY_LOCAL_MACHINE)
        try:
            serverInstKey = _winreg.OpenKey(r,
                r'SOFTWARE\VMware, Inc.\VMware Server'
              )
            try:
                serverInstDir = _winreg.QueryValueEx(
                    serverInstKey, 'InstallPath'
                  )[0]
            finally:
                _winreg.CloseKey(serverInstKey)
        finally:
            r.Close()

        vixDir = os.path.join(serverInstDir, os.pardir, 'VMware VIX')
        if vixDir.endswith(os.sep):
            vixDir = vixDir[:-len(os.sep)]
        vixDir = os.path.normpath(vixDir)

        # On Windows, find the spaceless version of the provided directory
        # name.  It's necessary to do this because the distutils support code
        # for MSVC quotes paths that contain spaces incorrectly.
        vixDir =  os.path.normcase(win32api.GetShortPathName(vixDir))

        includeDirs.append(vixDir)

        libNames.append('vix')
        libDirs.append(vixDir)

    findAndLoadVixLocations()

if CHECKED_BUILD:
    # Lack of a second tuple element is the distutils conventions for
    # "undefine this macro".
    macroDefs.append(('NDEBUG',))

if PLATFORM_IS_WINDOWS and 'mingw' not in (' '.join(sys.argv)).lower():
    # Using MSVC, not MinGW.

    # MSVC's /Wall generates warnings for just about everything one could
    # imagine (even just to indicate that it has inlined a function).  Some of
    # these are useful, but most aren't, so it's off by default.
    # extraCompilerArgs.append('/Wall')

    # Generate warnings for potential 64-bit portability problems (this option
    # is not available in MSVC 6):
    if not (sys.version_info < (2,4) or 'MSC V.1200 ' in sys.version.upper()):
        extraCompilerArgs.append('/Wp64')

    # Enable the pooling of read-only strings (reduces executable size):
    extraCompilerArgs.append('/GF')
else:
    # Assumes that the compiler is GCC.

    # Would like to enable pedantic and c99 options, but VMWare headers contain
    # imcompatible code:
    # extraCompilerArgs.append('-pedantic')
    # extraCompilerArgs.append('-std=c99')

    # By default, distutils includes the -fno-strict-aliasing flag on *nix-GCC,
    # but not on MinGW-GCC.
    extraCompilerArgs.append('-fno-strict-aliasing')

if not PLATFORM_IS_WINDOWS:
    includeDirs.append('/usr/include/vmware-vix')
    libNames.append('vixAllProducts')


extensionModules.append(
    dc.Extension('pyvix._vixmodule',
        sources=['_vixmodule.c'],
        libraries=libNames,
        include_dirs=includeDirs,
        library_dirs=libDirs,
        define_macros=macroDefs,
        extra_compile_args=extraCompilerArgs,
        extra_link_args=extraLinkerArgs,
      )
  )

dc.setup(
    name='pyvix',
    version=pyvixVersion,
    author='David S. Rushby',
    author_email='davidrushby@yahoo.com',
    # XXX:
    # url='http://',
    # description='',
    # long_description='',
    # license='',

    package_dir={'pyvix': os.curdir},

    py_modules=pythonModules,
    ext_modules=extensionModules,
    data_files=dataFiles,
  )

print '\nSucceeded:\n ', sys.executable, ' '.join(sys.argv)
