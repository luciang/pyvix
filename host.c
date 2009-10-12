/******************************************************************************
 * pyvix - Implementation of Host Class
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

static status initSupport_Host(void) {
  /* HostType is a new-style class, so PyType_Ready must be called before its
   * getters and setters will function. */
  if (PyType_Ready(&HostType) < 0) { goto fail; }

  return SUCCEEDED;
  fail:
    /* This function is indirectly called by the module loader, which makes no
     * provision for error recovery. */
    return FAILED;
} /* initSupport_Host */

#define HOST_REQUIRE_OPEN(host) \
  SHW_REQUIRE_OPEN((StatefulHandleWrapper *) (host))
#define Host_isOpen StatefulHandleWrapper_isOpen
#define Host_changeState(host, newState) \
  StatefulHandleWrapper_changeState((StatefulHandleWrapper *) (host), newState)

static PyObject *pyf_Host_new(
    PyTypeObject *subtype, PyObject *args, PyObject *kwargs
  )
{
  Host *self = (Host *) StatefulHandleWrapper_new(subtype);
  if (self == NULL) { goto fail; }

  /* Initialize Host-specific fields: */
  self->openVMs = NULL;

  return (PyObject *) self;
  fail:
    assert (PyErr_Occurred());
    assert (self == NULL);
    return NULL;
} /* pyf_Host_new */

static status Host_init(Host *self, PyObject *args, PyObject *kwargs) {
  status res = FAILED;
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err;

  static char* kwarg_list[] = {
      "hostType", "hostName", "hostPort", "username", "password", "options",
      NULL
    };
  VixServiceProvider hostType = VIX_SERVICEPROVIDER_VMWARE_SERVER;
  char *hostName = NULL;
  int hostPort = 0;
  char *username = NULL;
  char *password = NULL;
  VixHostOptions options = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|isissi", kwarg_list,
       &hostType, &hostName, &hostPort, &username, &password, &options
     ))
  { goto fail; }

  assert (self->handle == VIX_INVALID_HANDLE);
  LEAVE_PYTHON
  jobH = VixHost_Connect(VIX_API_VERSION,
      hostType, hostName, hostPort,
      username, password, options,

      VIX_INVALID_HANDLE, /* propertyListHandle */
      NULL, NULL /* callbackProc, clientData */
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_JOB_RESULT_HANDLE, &self->handle,
      VIX_PROPERTY_NONE
    );
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  assert (self->state == STATE_CREATED);
  if (Host_changeState(self, STATE_OPEN) != SUCCEEDED) { goto fail; }

  res = SUCCEEDED;
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (res == FAILED);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return res;
} /* Host_init */

static status Host_close(Host *self) {
  if (self->openVMs != NULL) {
    if (VMTracker_release(&self->openVMs) == SUCCEEDED) {
      assert (self->openVMs == NULL);
    } else {
      goto fail;
    }
  }

  if (self->state == STATE_OPEN && self->handle != VIX_INVALID_HANDLE) {
    LEAVE_PYTHON
    VixHost_Disconnect(self->handle);
    /* The Vix programming Guide says, "Since disconnecting the host unloads
     * the entire Vix server state, you should not call Vix_ReleaseHandle() on
     * any handle after you have called VixHost_Disconnect()." */
    self->handle = VIX_INVALID_HANDLE;
    ENTER_PYTHON
  }

  assert (self->handle == VIX_INVALID_HANDLE);
  if (Host_changeState(self, STATE_CLOSED) != SUCCEEDED) { goto fail; }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* Host_close */

static PyObject *pyf_Host_close(Host *self, PyObject *args) {
  HOST_REQUIRE_OPEN(self);
  if (Host_close(self) != SUCCEEDED) { goto fail; }
  Py_RETURN_NONE;
  fail:
    assert (PyErr_Occurred());
    return NULL;
} /* pyf_Host_close */

static PyObject *pyf_Host_closed_get(Host *self, void *closure) {
  return PyBool_FromLong(!Host_isOpen(self));
} /* pyf_Host_closed_get */

static status Host_delete(Host *self, bool allowedToRaise) {
  if (self->state == STATE_OPEN) {
    if (Host_close(self) != SUCCEEDED) {
      if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
    }
  }

  /* Should've already been cleared by Host_close: */
  assert (self->openVMs == NULL);

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* Host_delete */

static void pyf_Host___del__(Host *self) {
  Host_delete(self, false);

  /* Release the Host struct itself: */
  self->ob_type->tp_free((PyObject *) self);
} /* pyf_Host___del__ */

static PyObject *pyf_Host_findRunningVMPaths(Host *self) {
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  VixCallbackAccumulator acc;
  PyObject *res = NULL;

  HOST_REQUIRE_OPEN(self);

  if (VixCallbackAccumulator_ListInit(&acc) != SUCCEEDED) { goto fail; }

  LEAVE_PYTHON
  jobH = VixHost_FindItems(self->handle, VIX_FIND_RUNNING_VMS,
      VIX_INVALID_HANDLE, NO_TIMEOUT,
      VixCallback_accumulateStringList, &acc
    );
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  res = acc.target;
  acc.target = NULL;
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (res == NULL);
    VixCallbackAccumulator_clear(&acc);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return res;
} /* pyf_Host_findRunningVMPaths */

static PyObject *pyf_Host_registerOrUnregisterVM(Host *self, PyObject *args,
    bool shouldRegister
  )
{
  VixHandle jobH = VIX_INVALID_HANDLE;
  VixError err = VIX_OK;
  PyObject *res = NULL;

  char *vmxPath;

  HOST_REQUIRE_OPEN(self);
  if (!PyArg_ParseTuple(args, "s", &vmxPath)) { return NULL; }

  LEAVE_PYTHON
  if (shouldRegister) {
    jobH = VixHost_RegisterVM(self->handle, vmxPath, NULL, NULL);
  } else {
    jobH = VixHost_UnregisterVM(self->handle, vmxPath, NULL, NULL);
  }
  err = VixJob_Wait(jobH, VIX_PROPERTY_NONE);
  ENTER_PYTHON
  CHECK_VIX_ERROR(err);

  res = Py_None;
  Py_INCREF(Py_None);
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (res == NULL);
    /* Fall through to cleanup: */
  cleanup:
    if (jobH != VIX_INVALID_HANDLE) { Vix_ReleaseHandle(jobH); }
    return res;
} /* pyf_Host_registerVM */

static PyObject *pyf_Host_registerVM(Host *self, PyObject *args) {
  return pyf_Host_registerOrUnregisterVM(self, args, true);
} /* pyf_Host_registerVM */

static PyObject *pyf_Host_unregisterVM(Host *self, PyObject *args) {
  return pyf_Host_registerOrUnregisterVM(self, args, false);
} /* pyf_Host_registerVM */

static PyObject *pyf_Host_openVM(Host *self, PyObject *args) {
  PyObject *pyVMXPath;

  HOST_REQUIRE_OPEN(self);
  if (!PyArg_ParseTuple(args, "O!", &PyString_Type, &pyVMXPath)) {
    return NULL;
  }

  return PyObject_CallFunction((PyObject *) &VMType, "OO", self, pyVMXPath);
} /* pyf_Host_openVM */

static PyMethodDef Host_methods[] = {
    {"close",
        (PyCFunction) pyf_Host_close,
        METH_NOARGS
      },
    {"findRunningVMPaths",
        (PyCFunction) pyf_Host_findRunningVMPaths,
        METH_NOARGS
      },
    {"registerVM",
        (PyCFunction) pyf_Host_registerVM,
        METH_VARARGS
      },
    {"unregisterVM",
        (PyCFunction) pyf_Host_unregisterVM,
        METH_VARARGS
      },
    {"openVM",
        (PyCFunction) pyf_Host_openVM,
        METH_VARARGS
      },
    {NULL}  /* sentinel */
  };

static PyGetSetDef Host_getters_setters[] = {
    {"closed",
      (getter) pyf_Host_closed_get,
      NULL,
      "True if the connection to the Host is *known* to be closed."
    },
    {NULL}  /* sentinel */
  };

PyTypeObject HostType = { /* new-style class */
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "pyvix.vix.Host",                   /* tp_name */
    sizeof(Host),                       /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor) pyf_Host___del__,      /* tp_dealloc */
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

    Host_methods,                       /* tp_methods */
    NULL,                               /* tp_members */
    Host_getters_setters,               /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */

    (initproc) Host_init,               /* tp_init */
    0,                                  /* tp_alloc */
    pyf_Host_new,                       /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
    0,                                  /* tp_mro */
    0,                                  /* tp_cache */
    0,                                  /* tp_subclasses */
    0                                   /* tp_weaklist */
  };
