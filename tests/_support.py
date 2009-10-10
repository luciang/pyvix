import os, os.path, re, sys
import distutils.util

siteSpecificConfigDir = os.path.abspath(os.path.join(os.pardir, os.pardir))
sys.path.append(siteSpecificConfigDir)
try:
    import pyvix_test_site_config as site_config
except ImportError:
    print >> sys.stderr, ("Site-specific configuration file"
        " pyvix_test_site_config.py is missing from %s"
        % siteSpecificConfigDir
      )

site_config.generic_vmx = os.path.abspath(site_config.generic_vmx)


# Add the compiled pyvix directory that corresponds to this version of Python
# (assuming such a directory can be found) to Python's import path.
pyvixLibPath = None

def _guessLibLoc(dirNameShort, levelsUp):
    buildDir = os.path.abspath(os.path.join(
        *([os.pardir] * levelsUp + [dirNameShort, 'build'])
      ))
    libDir = 'lib.%s-%s' % (
        distutils.util.get_platform(),
        '.'.join( [str(n) for n in sys.version_info[:2]] )
      )
    libPath = os.path.join(buildDir, libDir)
    if os.path.exists(libPath):
        return libPath
    else:
        return None

pyvixLibPath = _guessLibLoc(os.curdir, levelsUp=1)

if pyvixLibPath and os.path.exists(pyvixLibPath):
    sys.path.insert(0, pyvixLibPath)

    # Also modify the PYTHONPATH environment variable, primarily so that
    # subprocesses launched by the main process will be able to import
    # pyvix without necessarily importing this module.
    if os.environ.has_key('PYTHONPATH'):
        if pyvixLibPath not in os.environ['PYTHONPATH'].split(os.pathsep):
            os.environ['PYTHONPATH'] = pyvixLibPath + os.pathsep + os.environ['PYTHONPATH']
    else:
        os.environ['PYTHONPATH'] = pyvixLibPath