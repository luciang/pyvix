/******************************************************************************
 * pyvix - Implementation of Snapshot Class
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

/* SnapshotTracker method declarations: */
static status SnapshotTracker_add(SnapshotTracker **list_slot, Snapshot *cont);
static status SnapshotTracker_remove(SnapshotTracker **list_slot,
    Snapshot *cont, bool
  );

#define Snapshot_REQUIRE_OPEN(snap) \
  SHW_REQUIRE_OPEN((StatefulHandleWrapper *) (snap))
#define Snapshot_isOpen StatefulHandleWrapper_isOpen
#define Snapshot_changeState(snap, newState) \
  StatefulHandleWrapper_changeState((StatefulHandleWrapper *) (snap), newState)


static status initSupport_Snapshot(void) {
  /* SnapshotType is a new-style class, so PyType_Ready must be called before
   * its getters and setters will function. */
  if (PyType_Ready(&SnapshotType) < 0) { goto fail; }

  return SUCCEEDED;
  fail:
    /* This function is indirectly called by the module loader, which makes no
     * provision for error recovery. */
    return FAILED;
} /* initSupport_Snapshot */

static PyObject *pyf_Snapshot_new(
    PyTypeObject *subtype, PyObject *args, PyObject *kwargs
  )
{
  Snapshot *self = (Snapshot *) StatefulHandleWrapper_new(subtype);
  if (self == NULL) { goto fail; }

  /* Initialize Snapshot-specific fields: */
  self->vm = NULL;

  return (PyObject *) self;
  fail:
    assert (PyErr_Occurred());
    assert (self == NULL);
    return NULL;
} /* pyf_Snapshot_new */

static status Snapshot_init(Snapshot *self, PyObject *args) {
  status res = FAILED;

  VM *vm;
  VixHandle snapH;

  if (!PyArg_ParseTuple(args, "O!" VixHandle_EXTRACTION_CODE,
        &VMType, &vm, &snapH)
     )
  { goto fail; }

  assert (self->vm == NULL);
  Py_INCREF(vm);
  self->vm = vm;

  assert (self->handle == VIX_INVALID_HANDLE);
  assert (snapH != VIX_INVALID_HANDLE);
  self->handle = snapH;

  assert (self->state == STATE_CREATED);
  if (Snapshot_changeState(self, STATE_OPEN) != SUCCEEDED) { goto fail; }

  /* Enter self in the VM's open Snapshot tracker: */
  if (SnapshotTracker_add(&vm->openSnapshots, self) != SUCCEEDED) {
    goto fail;
  }

  res = SUCCEEDED;
  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    assert (res == FAILED);
    /* Fall through to cleanup: */
  cleanup:
    return res;
} /* Snapshot_init */

#define Snapshot_clearVMReferences(snap) Py_CLEAR((snap)->vm)

#define Snapshot_hasBeenUntracked(snap) ((snap)->vm == NULL)

static status Snapshot_close_withoutUnlink(Snapshot *self, bool allowedToRaise) {
  if (self->state == STATE_OPEN && self->handle != VIX_INVALID_HANDLE) {
    LEAVE_PYTHON
    Vix_ReleaseHandle(self->handle);
    self->handle = VIX_INVALID_HANDLE;
    ENTER_PYTHON
  }

  assert (self->handle == VIX_INVALID_HANDLE);
  if (Snapshot_changeState(self, STATE_CLOSED) != SUCCEEDED) { goto fail; }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* Snapshot_close_withoutUnlink */

static status Snapshot_close_withUnlink(Snapshot *self, bool allowedToRaise) {
  /* Since the caller is asking us to unlink, self should still have a VM, and
   * self should be present in the VM's open Snapshot tracker. */
  assert (self->vm != NULL);
  assert (self->vm->openSnapshots != NULL);

  if (Snapshot_close_withoutUnlink(self, allowedToRaise) == SUCCEEDED) {
    assert (self->state == STATE_CLOSED);
  } else {
    if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
  }

  /* Remove self from the VM's open Snapshot tracker: */
  if (SnapshotTracker_remove(&self->vm->openSnapshots, self, true) != SUCCEEDED) {
    if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
  }

  Snapshot_clearVMReferences(self);

  assert (Snapshot_hasBeenUntracked(self));
  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* Snapshot_close_withUnlink */

static PyObject *pyf_Snapshot_close(Snapshot *self, PyObject *args) {
  Snapshot_REQUIRE_OPEN(self);
  if (Snapshot_close_withUnlink(self, true) != SUCCEEDED) { goto fail; }
  Py_RETURN_NONE;
  fail:
    assert (PyErr_Occurred());
    return NULL;
} /* pyf_Snapshot_close */

static PyObject *pyf_Snapshot_closed_get(Snapshot *self, void *closure) {
  return PyBool_FromLong(!Snapshot_isOpen(self));
} /* pyf_Snapshot_closed_get */

static status Snapshot_untrack(Snapshot *self, bool allowedToRaise) {
  if (Snapshot_close_withoutUnlink(self, allowedToRaise) != SUCCEEDED) {
    return FAILED;
  }
  assert (!Snapshot_isOpen(self));

  Snapshot_clearVMReferences(self);

  assert (Snapshot_hasBeenUntracked(self));
  return SUCCEEDED;
} /* Snapshot_untrack */

static status Snapshot_delete(Snapshot *self, bool allowedToRaise) {
  if (self->state == STATE_OPEN) {
    if (Snapshot_close_withUnlink(self, allowedToRaise) == SUCCEEDED) {
      assert (Snapshot_hasBeenUntracked(self));
    } else {
      if (allowedToRaise) { goto fail; } else { SUPPRESS_EXCEPTION; }
    }
  }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* Snapshot_delete */

static void pyf_Snapshot___del__(Snapshot *self) {
  Snapshot_delete(self, false);

  /* Release the Snapshot struct itself: */
  self->ob_type->tp_free((PyObject *) self);
} /* pyf_Snapshot___del__ */

static PyObject *pyf_Snapshot_vm_get(Snapshot *self, void *closure) {
  PyObject *vm = (self->vm != NULL ? (PyObject *) self->vm : Py_None);
  Py_INCREF(vm);
  return vm;
} /* pyf_Snapshot_vm_get */

static PyMethodDef Snapshot_methods[] = {
    {"close",
        (PyCFunction) pyf_Snapshot_close,
        METH_NOARGS
      },
    {NULL}  /* sentinel */
  };

static PyGetSetDef Snapshot_getters_setters[] = {
    {"closed",
        (getter) pyf_Snapshot_closed_get,
        NULL,
        "True if the Snapshot is *known* to be closed."
      },
    {"vm",
        (getter) pyf_Snapshot_vm_get,
        NULL,
        "The VM under which this Snapshot resides."
      },
    {NULL}  /* sentinel */
  };

PyTypeObject SnapshotType = { /* new-style class */
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "pyvix.vix.Snapshot",               /* tp_name */
    sizeof(Snapshot),                   /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor) pyf_Snapshot___del__,  /* tp_dealloc */
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

    Snapshot_methods,                   /* tp_methods */
    NULL,                               /* tp_members */
    Snapshot_getters_setters,           /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */

    (initproc) Snapshot_init,           /* tp_init */
    0,                                  /* tp_alloc */
    pyf_Snapshot_new,                   /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
    0,                                  /* tp_mro */
    0,                                  /* tp_cache */
    0,                                  /* tp_subclasses */
    0                                   /* tp_weaklist */
  };

/* SnapshotTracker support defs: */
LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_PYALLOC_NOQUAL(SnapshotTracker, Snapshot)
