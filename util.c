/******************************************************************************
 * pyvix - Miscellaneous Utility Code
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

static PyObject *pyf_extractProperty(VixHandle h, VixPropertyID propID) {
  /* Looks up the specified property propID on handle h, then converts the C
   * property value to an appropriate Python object. */
  PyObject *pyProp = NULL;
  VixError err = VIX_OK;
  VixPropertyType propType = VIX_PROPERTYTYPE_ANY;

  err = Vix_GetPropertyType(h, propID, &propType);
  CHECK_VIX_ERROR(err);

  switch (propType) {
    case VIX_PROPERTYTYPE_STRING: {
      char *s = NULL;
      err = Vix_GetProperties(h, propID, &s, VIX_PROPERTY_NONE);
      if (VIX_FAILED(err)) {
        autoRaiseVIXError(err);
        pyvix_vix_buffer_free(s);
        goto fail;
      } else {
        assert (s != NULL);
        pyProp = PyString_FromString(s);
        pyvix_vix_buffer_free(s);
        if (pyProp == NULL) { goto fail; }
      }
      break;
    }

    case VIX_PROPERTYTYPE_INTEGER: {
      int i;
      err = Vix_GetProperties(h, propID, &i, VIX_PROPERTY_NONE);
      CHECK_VIX_ERROR(err);
      pyProp = PyInt_FromLong(i);
      if (pyProp == NULL) { goto fail; }
      break;
    }

    case VIX_PROPERTYTYPE_INT64: {
      int64 i;
      err = Vix_GetProperties(h, propID, &i, VIX_PROPERTY_NONE);
      CHECK_VIX_ERROR(err);
      pyProp = PythonIntOrLongFrom64BitValue(i);
      if (pyProp == NULL) { goto fail; }
      break;
    }

    case VIX_PROPERTYTYPE_BOOL: {
      int i;
      err = Vix_GetProperties(h, propID, &i, VIX_PROPERTY_NONE);
      CHECK_VIX_ERROR(err);
      pyProp = PyBool_FromLong(i);
      if (pyProp == NULL) { goto fail; }
      break;
    }

    default:
      raiseNonNumericVIXError(VIXInternalError,
          "Unable to extract this property type."
        );
      goto fail;
  }

  assert (!PyErr_Occurred());
  assert (pyProp != NULL);
  return pyProp;
  fail:
    assert (PyErr_Occurred());
    Py_XDECREF(pyProp);
    return NULL;
} /* pyf_extractProperty */
