import py.test

import _support
from pyvix.vix import *

def test_Host_connect_noArgs():
    # With no args specified, Host should be able to connect to the local
    # VMWare Server without any explicit security information.
    h = Host()

def test_Host_connect_badSecurityInfo():
    # With the host name specified but security information missing (or bogus),
    # should fail with VIXSecurityException:
    py.test.raises(VIXSecurityException, Host,
        hostName='localhost',
      )
    py.test.raises(VIXSecurityException, Host,
        hostName='localhost',
        username='BOGUS_USERNAME', password='BOGUS_PASSWORD',
      )

def test_Host_close_explicit():
    h = Host()
    assert not h.closed
    h.close()
    assert h.closed
    py.test.raises(VIXClientProgrammerError, h.close)

def test_Host_findRunningVMs():
    h = Host()
    vmPaths = h.findRunningVMPaths()
    if len(vmPaths) > 0:
        assert isinstance(vmPaths[0], basestring)

    print 'Here are the paths of the VMs that are running on this host:'
    print vmPaths

def test_Host_registerAndUnregisterVM():
    VM_PATH = _support.site_config.generic_vmx

    h = Host()

    h.unregisterVM(VM_PATH)
    # Unregistering shouldn't object if the VM isn't registered in the first
    # place:
    h.unregisterVM(VM_PATH)

    h.registerVM(VM_PATH)
    try:
        h.unregisterVM(VM_PATH)
    finally:
        h.registerVM(VM_PATH)