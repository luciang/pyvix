/******************************************************************************
 * pyvix - Implementation of StatefulHandleWrapper "Unofficial Superclass"
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

static status StatefulHandleWrapper_changeState(StatefulHandleWrapper *self,
    GenericState newState
  )
{
  switch (newState) {
    case STATE_CREATED:
      raiseNonNumericVIXError(VIXInternalError,
          "Should never enter STATE_CREATED after creation phase."
        );
      goto fail;

    case STATE_OPEN:
      if (self->state != STATE_CREATED) {
        raiseNonNumericVIXError(VIXInternalError,
          "Should only enter STATE_OPEN from STATE_CREATED."
        );
      }
      break;

    case STATE_CLOSED:
      if (self->state == STATE_CLOSED) {
        raiseNonNumericVIXError(VIXInternalError,
          "Cannot enter STATE_CLOSED, because already closed."
        );
      }
      break;
  }

  self->state = newState;
  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* StatefulHandleWrapper_changeState */

#define StatefulHandleWrapper_isOpen(shw) ((shw)->state == STATE_OPEN)

#define SHW_REQUIRE_OPEN_WITH_FAILURE(shw, failure_action) \
  if (StatefulHandleWrapper_require_open(shw) != SUCCEEDED) { failure_action; }

#define SHW_REQUIRE_OPEN(shw) \
  SHW_REQUIRE_OPEN_WITH_FAILURE(shw, return NULL)

static status StatefulHandleWrapper_require_open(StatefulHandleWrapper *self) {
  assert (self != NULL);

  if (!StatefulHandleWrapper_isOpen(self)) {
    raiseNonNumericVIXError(VIXClientProgrammerError,
        "The object must be OPEN to perform this operation."
      );
    goto fail;
  }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* StatefulHandleWrapper_require_open */

static StatefulHandleWrapper *StatefulHandleWrapper_new(PyTypeObject *subtype) {
  StatefulHandleWrapper *self =
    (StatefulHandleWrapper *) subtype->tp_alloc(subtype, 0);
  if (self == NULL) { goto fail; }

  /* This is a rare instance where it's acceptable to bypass
   * StatefulHandleWrapper_changeState when changing the object's state (the
   * state field was previously uninitialized, so there's no way
   * StatefulHandleWrapper_changeState could validate the transition). */
  self->state = STATE_CREATED;
  self->handle = VIX_INVALID_HANDLE;

  return self;
  fail:
    assert (PyErr_Occurred());
    assert (self == NULL);
    return NULL;
} /* StatefulHandleWrapper_new */

static int StatefulHandleWrapper_length(StatefulHandleWrapper *self) {
  /* The concept of length doesn't even make sense in this context, but it
   * wouldn't be a good idea to return zero, which would be taken to mean that
   * there are no entries available. */
  return 1;
} /* StatefulHandleWrapper_length */

static PyObject *StatefulHandleWrapper_subscript(StatefulHandleWrapper *self,
    PyObject *key
  )
{
  /* This function implements the operator that's used to look up property
   * values on an object that encapsulates a VixHandle.  An example is
   * vm[VIX_PROPERTY_VM_POWER_STATE], where vm is an instance of
   * pyvix.vix.VM. */
  PyObject *pyValue = NULL;
  const long intPropID = PyInt_AsLong(key);
  if (PyErr_Occurred()) { goto fail; }

  {
    const VixPropertyID propID = (VixPropertyID) intPropID;
    pyValue = pyf_extractProperty(self->handle, propID);
    if (pyValue == NULL) { goto fail; }
  }

  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    Py_CLEAR(pyValue);
    /* Fall through to cleanup: */
  cleanup:
    return pyValue;
} /* StatefulHandleWrapper_subscript */

static int StatefulHandleWrapper_ass_sub(StatefulHandleWrapper *self,
    PyObject *v, PyObject *w
  )
{
  /* XXX: Should implement property assignment as appropriate. */
  raiseNonNumericVIXError(VIXInternalError,
      "Property assignment is not yet implemented."
    );
  return -1;
} /* StatefulHandleWrapper_ass_sub */

static PyMappingMethods StatefulHandleWrapper_as_mapping = {
    (inquiry) StatefulHandleWrapper_length, /* mp_length */
    (binaryfunc) StatefulHandleWrapper_subscript, /* mp_subscript */
    (objobjargproc) StatefulHandleWrapper_ass_sub, /* mp_ass_subscript */
  };
