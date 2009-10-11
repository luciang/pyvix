/******************************************************************************
 * pyvix - Implementation of VM Class
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

/* VMTracker method declarations: */
static status VMTracker_add(VMTracker **list_slot, VM *cont);
static status VMTracker_remove(VMTracker **list_slot, VM *cont, bool);

#define VM_REQUIRE_OPEN(vm) \
  SHW_REQUIRE_OPEN((StatefulHandleWrapper *) (vm))
#define VM_isOpen StatefulHandleWrapper_isOpen
#define VM_changeState(vm, newState) \
  StatefulHandleWrapper_changeState((StatefulHandleWrapper *) (vm), newState)


static status initSupport_VM(void) {
  /* VMType is a new-style class, so PyType_Ready must be called before its
   * getters and setters will function. */
  if (PyType_Ready(&VMType) < 0) { goto fail; }

  return SUCCEEDED;
  fail:
    /* This function is indirectly called by the module loader, which makes no
     * provision for error recovery. */
    return FAILED;
} /* initSupport_VM */

static PyObject *pyf_VM_new(
    PyTypeObject *subtype, PyObject *args, PyObject *kwargs
  )
{
  VM *self = (VM *) StatefulHandleWrapper_new(subtype);
  if (self == NULL) { goto fail; }

  /* Initialize VM-specific fields: */
  self->host = NULL;
  self->vmxPath = NULL;

  return (PyObject *) self;
  fail:
    assert (PyErr_Occurred());
    assert (self == NULL);
    return NULL;
} /* pyf_VM_new */

static status VM_init(VM *self, PyObject *args) {
  status res = FAILED;
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;

  Host *host;
  char *vmxPath;

  if (!PyArg_ParseTuple(args, "O!s", &HostType, &host, &vmxPath)) { goto fail; }

  assert (self->host == NULL);
  Py_INCREF(host);
  self->host = host;
  assert (self->vmxPath == NULL);
  assert (self->handle == VIX_INVALID_HANDLE);

  LEAVE_PYTHON
  self->vmxPath = strdup(vmxPath);
  ENTER_PYTHON

  if (self->vmxPath == NULL) { Py_DECREF(host); self->host = NULL; goto fail; }

  LEAVE_PYTHON
  jobH = VixVM_Open(host->handle, vmxPath, NULL, NULL);
  err = VixJob_Wait(jobH, VIX_PROPERTY_JOB_RESULT_HANDLE, &self->handle,
      VIX_PROPERTY_NONE
    );
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  assert (self->state == STATE_CREATED);
  if (VM_changeState(self, STATE_OPEN) != SUCCEEDED) { goto fail; }

  /* Enter self in the host's open VM tracker: */
  if (VMTracker_add(&host->openVMs, self) != SUCCEEDED) { goto fail; }

  res = SUCCEEDED;
  goto cleanup;
  fail:
    if(self && self->vmxPath)
      free(self->vmxPath);
    assert (PyErr_Occurred());
    assert (res == FAILED);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return res;
} /* VM_init */

#define VM_clearHostReferences(vm) Py_CLEAR((vm)->host)

#define VM_hasBeenUntracked(vm) ((vm)->host == NULL)

static status VM_close_withoutUnlink(VM *self, bool allowedToRaise) {
  if (self->openSnapshots != NULL) {
    if (SnapshotTracker_release(&self->openSnapshots) == SUCCEEDED) {
      assert (self->openSnapshots == NULL);
    } else {
      if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
    }
  }

  if (self->state == STATE_OPEN && self->handle != VIX_INVALID_HANDLE) {
    LEAVE_PYTHON
    Vix_ReleaseHandle(self->handle);
    self->handle = VIX_INVALID_HANDLE;
    ENTER_PYTHON
  }

  assert (self->handle == VIX_INVALID_HANDLE);
  if (VM_changeState(self, STATE_CLOSED) != SUCCEEDED) { goto fail; }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* VM_close_withoutUnlink */

static status VM_close_withUnlink(VM *self, bool allowedToRaise) {
  /* Since the caller is asking us to unlink, self should still have a host,
   * and self should be present in the host's open VM tracker. */
  assert (self->host != NULL);
  assert (self->host->openVMs != NULL);

  if (VM_close_withoutUnlink(self, allowedToRaise) == SUCCEEDED) {
    assert (self->state == STATE_CLOSED);
  } else {
    if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
  }

  /* Remove self from the host's open VM tracker: */
  if (VMTracker_remove(&self->host->openVMs, self, true) != SUCCEEDED) {
    if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
  }

  VM_clearHostReferences(self);
  free(self->vmxPath); self->vmxPath = NULL;

  assert (VM_hasBeenUntracked(self));
  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* VM_close_withUnlink */

static PyObject *pyf_VM_close(VM *self, PyObject *args) {
  VM_REQUIRE_OPEN(self);
  if (VM_close_withUnlink(self, true) != SUCCEEDED) { goto fail; }
  Py_RETURN_NONE;
  fail:
    assert (PyErr_Occurred());
    return NULL;
} /* pyf_VM_close */

static PyObject *pyf_VM_closed_get(VM *self, void *closure) {
  return PyBool_FromLong(!VM_isOpen(self));
} /* pyf_VM_closed_get */

static status VM_untrack(VM *self, bool allowedToRaise) {
  if (VM_close_withoutUnlink(self, allowedToRaise) != SUCCEEDED) {
    return FAILED;
  }
  assert (!VM_isOpen(self));

  VM_clearHostReferences(self);
  free(self->vmxPath); self->vmxPath = NULL;
  assert (VM_hasBeenUntracked(self));

  return SUCCEEDED;
} /* VM_untrack */

static status VM_delete(VM *self, bool allowedToRaise) {
  if (self->state == STATE_OPEN) {
    if (VM_close_withUnlink(self, allowedToRaise) == SUCCEEDED) {
      assert (VM_hasBeenUntracked(self));
    } else {
      if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
    }
  }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* VM_delete */

static void pyf_VM___del__(VM *self) {
  VM_delete(self, false);

  /* Release the VM struct itself: */
  self->ob_type->tp_free((PyObject *) self);
} /* pyf_VM___del__ */

static PyObject *pyf_VM_powerOnOrOff(VM *self, bool shouldPowerOn) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  if (shouldPowerOn) {
    jobH = VixVM_PowerOn(self->handle, VIX_VMPOWEROP_NORMAL,
        VIX_INVALID_HANDLE, NULL, NULL
      );
  } else {
    jobH = VixVM_PowerOff(self->handle, 0, NULL, NULL);
  }
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_powerOn */

static PyObject *pyf_VM_powerOn(VM *self) {
  return pyf_VM_powerOnOrOff(self, true);
} /* pyf_VM_powerOn */

static PyObject *pyf_VM_powerOff(VM *self) {
  return pyf_VM_powerOnOrOff(self, false);
} /* pyf_VM_powerOn */

static PyObject *pyf_VM_reset(VM *self) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  jobH = VixVM_Reset(self->handle,
      /* powerOnOptions:  Must be VIX_VMPOWEROP_NORMAL in current release: */
      VIX_VMPOWEROP_NORMAL,
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_reset */

static PyObject *pyf_VM_suspend(VM *self) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  jobH = VixVM_Suspend(self->handle,
      /* powerOffOptions:  Must be VIX_VMPOWEROP_NORMAL in current release: */
      VIX_VMPOWEROP_NORMAL,
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_suspend */

static PyObject *pyf_VM_upgradeVirtualHardware(VM *self) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  jobH = VixVM_UpgradeVirtualHardware(self->handle,
      0, /* options:  Must be 0 in current release. */
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_upgradeVirtualHardware */

static PyObject *pyf_VM_waitForToolsInGuest(VM *self, PyObject *args) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;
  VixToolsState toolsState = VIX_TOOLSSTATE_UNKNOWN;

  int timeoutSecs = NO_TIMEOUT;

  VM_REQUIRE_OPEN(self);
  if (!PyArg_ParseTuple(args, "|i", &timeoutSecs)) { goto fail; }

  LEAVE_PYTHON
  /* XXX: As of VMWare Server 1.0RC1, the timeout either doesn't work, or
   * requires the use of the async callback instead of VixJob_Wait.  At any
   * rate, the timeout doesn't work as expected at present. */
  jobH = VixVM_WaitForToolsInGuest(self->handle, timeoutSecs, NULL, NULL);
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  LEAVE_PYTHON
  err = Vix_GetProperties(self->handle, VIX_PROPERTY_VM_TOOLS_STATE,
      &toolsState, VIX_PROPERTY_NONE
    );
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  /* If the VM's "tools state" is still undefined even after the VixJob_Wait
   * call returned, then we timed out. */
  pyRes = PyBool_FromLong(toolsState != VIX_TOOLSSTATE_UNKNOWN);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_waitForToolsInGuest */

static PyObject *pyf_VM_installTools(VM *self) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  jobH = VixVM_InstallTools(self->handle,
      0, /* options:  Must be 0 in current release. */
      NULL, /* commandLineArgs:  Must be NULL in current release. */
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_installTools */

static PyObject *pyf_VM_delete(VM *self) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *pyRes = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  jobH = VixVM_Delete(self->handle,
      0, /* deleteOptions:  Must be 0 in current release. */
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_delete */

static PyObject *pyf_VM_createSnapshot(VM *self,
    PyObject *args, PyObject *kwargs
  )
{
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;
  PyObject *pySnap = NULL;
  VixHandle snapH;

  static char* kwarg_list[] = {"name", "description", "options", NULL};
  char *name = NULL;
  char *description = NULL;
  int options = 0;

  VM_REQUIRE_OPEN(self);

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ssi", kwarg_list,
       &name, &description, &options
     ))
  { goto fail; }

  LEAVE_PYTHON
  jobH = VixVM_CreateSnapshot(self->handle,
      name, description, options,
      /* propertyListHandle:  Must be VIX_INVALID_HANDLE in current release: */
      VIX_INVALID_HANDLE,
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH,
      VIX_PROPERTY_JOB_RESULT_HANDLE, &snapH,
      VIX_PROPERTY_NONE
    );
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  assert (snapH != VIX_INVALID_HANDLE);
  pySnap = PyObject_CallFunction((PyObject *) &SnapshotType,
      "O" VixHandle_FUNCTION_CALL_CODE, self, snapH
    );
  /* If the creation of pySnap succeeded, the Snapshot instance now owns snapH;
   * if the creation failed, we need to release snapH: */
  if (pySnap == NULL) {
    Vix_ReleaseHandle(snapH);
    goto fail;
  }

  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    Py_XDECREF(pySnap);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pySnap;
} /* pyf_VM_createSnapshot */

static PyObject *pyf_VM_removeSnapshot(VM *self, PyObject *args) {
  PyObject *pyRes = NULL;
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;

  Snapshot *pySnap;

  VM_REQUIRE_OPEN(self);

  if (!PyArg_ParseTuple(args, "O!", &SnapshotType, &pySnap)) { goto fail; }

  LEAVE_PYTHON
  jobH = VixVM_RemoveSnapshot(self->handle,
      pySnap->handle,
      0, /* options:  Must be 0 in current release. */
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_removeSnapshot */

static PyObject *pyf_VM_revertToSnapshot(VM *self, PyObject *args) {
  PyObject *pyRes = NULL;
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;

  Snapshot *pySnap;

  VM_REQUIRE_OPEN(self);

  if (!PyArg_ParseTuple(args, "O!", &SnapshotType, &pySnap)) { goto fail; }

  LEAVE_PYTHON
  jobH = VixVM_RevertToSnapshot(self->handle,
      pySnap->handle,
      0, /* options:  Must be 0 in current release. */
      /* propertyListHandle:  Must be VIX_INVALID_HANDLE in current release: */
      VIX_INVALID_HANDLE,
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_revertToSnapshot */

static PyObject *pyf_VM_loginInGuest(VM *self,
    PyObject *args, PyObject *kwargs
  )
{
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;
  PyObject *pyRes = NULL;

  static char* kwarg_list[] = {"username", "password", "options", NULL};
  char *username = NULL;
  char *password = NULL;
  int options = 0;

  VM_REQUIRE_OPEN(self);

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|i", kwarg_list,
       &username, &password, &options
     ))
  { goto fail; }

  LEAVE_PYTHON
  jobH = VixVM_LoginInGuest(self->handle,
      username, password, options,
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_loginInGuest */

static PyObject *pyf_VM_copyFile(VM *self, PyObject *args,
    bool fromHostToGuest
  )
{
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;
  PyObject *pyRes = NULL;

  char *src;
  char *dest;

  VM_REQUIRE_OPEN(self);

  if (!PyArg_ParseTuple(args, "ss", &src, &dest)) { goto fail; }

  LEAVE_PYTHON
  if (fromHostToGuest) {
    jobH = VixVM_CopyFileFromHostToGuest(self->handle,
        src, dest,
        0, /* options:  Must be 0 in current release. */
        /* propertyList:  Must be VIX_INVALID_HANDLE in current release: */
        VIX_INVALID_HANDLE,
        NULL, /* callbackProc */
        NULL  /* clientData */
      );
  } else {
    jobH = VixVM_CopyFileFromGuestToHost(self->handle,
        src, dest,
        0, /* options:  Must be 0 in current release. */
        /* propertyList:  Must be VIX_INVALID_HANDLE in current release: */
        VIX_INVALID_HANDLE,
        NULL, /* callbackProc */
        NULL  /* clientData */
      );
  }
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_copyFile */

static PyObject *pyf_VM_copyFileFromHostToGuest(VM *self, PyObject *args) {
  return pyf_VM_copyFile(self, args, true);
} /* pyf_VM_copyFileFromHostToGuest */

static PyObject *pyf_VM_copyFileFromGuestToHost(VM *self, PyObject *args) {
  return pyf_VM_copyFile(self, args, false);
} /* pyf_VM_copyFileFromGuestToHost */

static PyObject *pyf_VM_runProgramInGuest(VM *self, PyObject *args) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;
  PyObject *pyRes = NULL;

  char *progPath;
  char *commandLine;

  VM_REQUIRE_OPEN(self);

  if (!PyArg_ParseTuple(args, "ss", &progPath, &commandLine)) { goto fail; }

  LEAVE_PYTHON
  jobH = VixVM_RunProgramInGuest(self->handle,
      progPath, commandLine,
      0, /* options */
      /* propertyList:  Must be VIX_INVALID_HANDLE in current release: */
      VIX_INVALID_HANDLE,
      NULL, /* callbackProc */
      NULL  /* clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  pyRes = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (pyRes == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return pyRes;
} /* pyf_VM_runProgramInGuest */

static PyObject *pyf_VM_host_get(VM *self, void *closure) {
  PyObject *host = (self->host != NULL ? (PyObject *) self->host : Py_None);
  Py_INCREF(host);
  return host;
} /* pyf_VM_host_get */

static PyObject *pyf_VM_nRootSnapshots_get(VM *self, void *closure) {
  VixError err = VIX_OK;
  int nRootSnapshots;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  err = VixVM_GetNumRootSnapshots(self->handle, &nRootSnapshots);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  return PyInt_FromLong(nRootSnapshots);
  fail:
    assert (PyErr_Occurred());
    return NULL;
} /* pyf_VM_nRootSnapshots_get */

static PyObject *pyf_VM_rootSnapshots_get(VM *self, void *closure) {
  VixError err = VIX_OK;
  int nRootSnapshots = -1;
  int i;
  PyObject *pySnapList = NULL;

  VM_REQUIRE_OPEN(self);

  LEAVE_PYTHON
  err = VixVM_GetNumRootSnapshots(self->handle, &nRootSnapshots);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  assert (nRootSnapshots >= 0);
  pySnapList = PyList_New(nRootSnapshots);
  if (pySnapList == NULL) { goto fail; }

  for (i = 0; i < nRootSnapshots; i++) {
    VixHandle snapH = VIX_INVALID_HANDLE;
    PyObject *pySnap = NULL;

    LEAVE_PYTHON
    err = VixVM_GetRootSnapshot(self->handle, i, &snapH);
    ENTER_PYTHON
    CHECK_VIX_ERROR(err);

    assert (snapH != VIX_INVALID_HANDLE);
    pySnap = PyObject_CallFunction((PyObject *) &SnapshotType,
        "Oi", self, snapH
      );
    /* If the creation of pySnap succeeded, the Snapshot instance now owns
     * snapH; if the creation failed, we need to release snapH: */
    if (pySnap == NULL) {
      Vix_ReleaseHandle(snapH);
      goto fail;
    }

    /* PyList_SET_ITEM steals our reference to pySnap: */
    PyList_SET_ITEM(pySnapList, i, pySnap);
  }

  assert (pySnapList != NULL);
  assert (PyList_GET_SIZE(pySnapList) == (Py_ssize_t) nRootSnapshots);
  return pySnapList;
  fail:
    assert (PyErr_Occurred());
    Py_XDECREF(pySnapList);
    return NULL;
} /* pyf_VM_rootSnapshots_get */


static PyObject *pyf_VM_vmxPath_get(VM *self, void *closure) {
  PyObject * pyVmxPath;

  if (self->vmxPath == NULL)
    return Py_None;

  pyVmxPath = Py_BuildValue("s", self->vmxPath);
  if (pyVmxPath == NULL)
    return Py_None;
  return pyVmxPath;
} /* pyf_VM_vmxPath_get */


static PyMethodDef VM_methods[] = {
    {"close",
        (PyCFunction) pyf_VM_close,
        METH_NOARGS
      },
    {"powerOn",
        (PyCFunction) pyf_VM_powerOn,
        METH_NOARGS
      },
    {"powerOff",
        (PyCFunction) pyf_VM_powerOff,
        METH_NOARGS
      },
    {"reset",
        (PyCFunction) pyf_VM_reset,
        METH_NOARGS
      },
    {"suspend",
        (PyCFunction) pyf_VM_suspend,
        METH_NOARGS
      },
    {"installTools",
        (PyCFunction) pyf_VM_installTools,
        METH_NOARGS
      },
    {"waitForToolsInGuest",
        (PyCFunction) pyf_VM_waitForToolsInGuest,
        METH_VARARGS
      },
    {"upgradeVirtualHardware",
        (PyCFunction) pyf_VM_upgradeVirtualHardware,
        METH_NOARGS
      },
    {"delete",
        (PyCFunction) pyf_VM_delete,
        METH_NOARGS
      },
    {"createSnapshot",
        /* It should actually be PyCFunctionWithKeywords, but GCC grumbles
         * about that: */
        (PyCFunction) pyf_VM_createSnapshot,
        METH_VARARGS | METH_KEYWORDS,
      },
    {"removeSnapshot",
        (PyCFunction) pyf_VM_removeSnapshot,
        METH_VARARGS
      },
    {"revertToSnapshot",
        (PyCFunction) pyf_VM_revertToSnapshot,
        METH_VARARGS
      },
    {"loginInGuest",
        /* It should actually be PyCFunctionWithKeywords, but GCC grumbles
         * about that: */
        (PyCFunction) pyf_VM_loginInGuest,
        METH_VARARGS | METH_KEYWORDS,
      },
    {"copyFileFromHostToGuest",
        (PyCFunction) pyf_VM_copyFileFromHostToGuest,
        METH_VARARGS
      },
    {"copyFileFromGuestToHost",
        (PyCFunction) pyf_VM_copyFileFromGuestToHost,
        METH_VARARGS
      },
    {"runProgramInGuest",
        (PyCFunction) pyf_VM_runProgramInGuest,
        METH_VARARGS
      },
    {NULL}  /* sentinel */
  };

static PyGetSetDef VM_getters_setters[] = {
    {"closed",
        (getter) pyf_VM_closed_get,
        NULL,
        "True if the VM is *known* to be closed."
      },
    {"host",
        (getter) pyf_VM_host_get,
        NULL,
        "The Host on which this VM resides."
      },
    {"nRootSnapshots",
        (getter) pyf_VM_nRootSnapshots_get,
        NULL,
        "The number of root snapshots in this VM (the length of the list"
        " returned by the rootSnapshots property)."
      },
    {"rootSnapshots",
        (getter) pyf_VM_rootSnapshots_get,
        NULL,
        "A list of Snapshot objects that represent the root snapshots in this"
        " VM."
      },
    {"vmxPath",
       (getter) pyf_VM_vmxPath_get,
        NULL,
        "The path to the corresponding vmx for this virtual machine."
      },
    {NULL}  /* sentinel */
  };

PyTypeObject VMType = { /* new-style class */
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "pyvix.vix.VM",                     /* tp_name */
    sizeof(VM),                         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor) pyf_VM___del__,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    &StatefulHandleWrapper_as_mapping,  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                        /* tp_flags */
    0,                                  /* tp_doc */
    0,		                              /* tp_traverse */
    0,		                              /* tp_clear */
    0,		                              /* tp_richcompare */
    0,		                              /* tp_weaklistoffset */

    0,                    		          /* tp_iter */
    0,		                              /* tp_iternext */

    VM_methods,                         /* tp_methods */
    NULL,                               /* tp_members */
    VM_getters_setters,                 /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */

    (initproc) VM_init,                 /* tp_init */
    0,                                  /* tp_alloc */
    pyf_VM_new,                         /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
    0,                                  /* tp_mro */
    0,                                  /* tp_cache */
    0,                                  /* tp_subclasses */
    0                                   /* tp_weaklist */
  };

/* VMTracker support defs: */
LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_PYALLOC_NOQUAL(VMTracker, VM)
