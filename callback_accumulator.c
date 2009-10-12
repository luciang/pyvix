/******************************************************************************
 * pyvix - Implementation of VixCallbackAccumulator Type
 * Copyright 2006 David S. Rushby
* Copyright 2008 Adam Pridgen Adam.Pridgen@[foundstone.com || gmail.com]
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

static status VixCallbackAccumulator_ListInit(VixCallbackAccumulator *acc) {
  assert (acc != NULL);

  acc->target = PyList_New(0);
  if (acc->target == NULL) { goto fail; }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* VixCallbackAccumulator_ListInit */

static status VixCallbackAccumulator_TupleInit(VixCallbackAccumulator *acc, unsigned int items) {
  assert (acc != NULL);

  acc->target = PyTuple_New(items);
  if (acc->target == NULL) { goto fail; }

  return SUCCEEDED;
  fail:
    assert (PyErr_Occurred());
    return FAILED;
} /* VixCallbackAccumulator_Tuplenit */


static status VixCallbackAccumulator_clear(VixCallbackAccumulator *acc) {
  assert (acc != NULL);

  Py_CLEAR(acc->target);

  return SUCCEEDED;
} /* VixCallbackAccumulator_clear */

static void VixCallback_accumulateStringList(VixHandle jobH,
    VixEventType eventType, VixHandle eventInfo, void *clientData
  )
{
  PyGILState_STATE gstate;
  PyObject *pyStr = NULL;
  /* Note:  clientData is typically a pointer to memory on another thread's
   * stack, so it's imperative that we refrain from accessing it IN ANY WAY
   * (even just to cast it to VixCallbackAccumulator *acc) until we're sure
   * that the other thread is still waiting for this thread to finish. */
  VixCallbackAccumulator *acc = NULL;

  /* Ignore event types that don't indicate that an item has been found: */
  if (eventType != VIX_EVENTTYPE_FIND_ITEM) { return; }

  ENTER_PYTHON_WITHOUT_CODE_BLOCK(gstate);

  acc = (VixCallbackAccumulator *) clientData;
  assert (acc != NULL);
  assert (acc->target != NULL);
  assert (PyList_CheckExact(acc->target));

  assert (eventInfo != VIX_INVALID_HANDLE);
  pyStr = pyf_extractProperty(eventInfo, VIX_PROPERTY_FOUND_ITEM_LOCATION);
  if (pyStr == NULL) { goto fail; }

  if (PyList_Append(acc->target, pyStr) != 0) { goto fail; }

  goto cleanup;
  fail:
    assert (PyErr_Occurred());
    /* Fall through to cleanup: */
  cleanup:
    Py_XDECREF(pyStr);
    LEAVE_PYTHON_WITHOUT_CODE_BLOCK(gstate);
} /* VixCallback_accumulateStringList */
