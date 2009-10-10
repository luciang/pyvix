# pyvix - Miscellaneous Utility Code
# Copyright 2006 David S. Rushby
# Available under the MIT license (see docs/license.txt for details).

import os.path, sys

PLATFORM_IS_WINDOWS = sys.platform.lower().startswith('win')

def findVixDir():
    if PLATFORM_IS_WINDOWS:
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
        return vixDir
