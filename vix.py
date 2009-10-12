#!/usr/bin/env python

# pyvix - Main Entry Point to VIX Functionality
# Copyright 2006 David S. Rushby
# Available under the MIT license (see docs/license.txt for details).

__timestamp__ = '2009.10.12.08.02.59.UTC'

import os

import _support

if _support.PLATFORM_IS_WINDOWS:
    # Look up the directory that contains the VIX DLL and insert it into the
    # PATH environment variable.
    _vixDir = _support.findVixDir()
    os.environ['PATH'] = _vixDir + os.pathsep + os.environ['PATH']

import _vixmodule as _v # The underlying C module.

# Load the essential VIX C API constants into the namespace of this module:
_v.initSupport_Constants(globals())

# Exceptions:
VIXException             = _v.VIXException
VIXInternalError         = _v.VIXInternalError
VIXSecurityException     = _v.VIXSecurityException
VIXClientProgrammerError = _v.VIXClientProgrammerError

# Main classes:
Host = _v.Host
VM = _v.VM
Snapshot = _v.Snapshot
