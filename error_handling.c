/******************************************************************************
 * pyvix - Error Handling Infrastructure
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

/* Convenience macros: */
#define CHECK_VIX_ERROR_AND(err, failure_action) \
  if (VIX_FAILED(err)) { \
    autoRaiseVIXError(err); \
    failure_action; \
  }

#define CHECK_VIX_ERROR(err) \
  CHECK_VIX_ERROR_AND(err, goto fail)

/* Pointers to pyvix exception types: */
static PyObject *VIXException             = NULL;
/* VIXInternalError applies when the internals of pyvix have become corrupt: */
static PyObject *VIXInternalError         = NULL;
static PyObject *VIXSecurityException     = NULL;
static PyObject *VIXClientProgrammerError = NULL;

static status initSupport_errorHandling(PyObject *vixModule) {
  #define DEFINE_EXC(targetVar, superType) \
    targetVar = PyErr_NewException("pyvix.vix." #targetVar, superType, NULL); \
    if (targetVar == NULL) { goto fail; } \
    if (PyModule_AddObject(vixModule, #targetVar, targetVar) != 0) { goto fail; }

  DEFINE_EXC(VIXException, PyExc_Exception);

  DEFINE_EXC(VIXInternalError,         VIXException);
  DEFINE_EXC(VIXSecurityException,     VIXException);
  DEFINE_EXC(VIXClientProgrammerError, VIXException);

  return SUCCEEDED;
  fail:
    /* This function is indirectly called by the module loader, which makes no
     * provision for error recovery. */
    return FAILED;
} /* initSupport_errorHandling */

static status raiseVIXError(VixError err, PyObject *excType, char *errText) {
  /* The VMWare Server 1.0 docs say that "only US English is supported in this
   * release", so the locale arg of Vix_GetErrorText is always NULL. */
  if (errText == NULL) {
    /* If no error text was specified, it's crucial that there be a VIX error
     * code from which we can derive error text. */
    assert (err != VIX_OK);
    errText = (char *) Vix_GetErrorText(err, NULL);
  }

  if (excType == NULL) { excType = VIXException; }
  PyErr_SetString(excType, errText);
  return SUCCEEDED;
} /* raiseVIXError */

static status raiseNonNumericVIXError(PyObject *excType, char *errText) {
  /* In this context, VIX_OK means "error code not specified" rather than
   * "there is no error". */
  return raiseVIXError(VIX_OK, excType, errText);
} /* raiseNonNumericVIXError */

static status autoRaiseVIXError(VixError err) {
  /* XXX:  Need to review all entries in the VIX_E_* enumeration and account
   * for them here. */
  PyObject *excType = NULL;

  switch (err) {
    case VIX_E_HOST_USER_PERMISSIONS:
    case VIX_E_GUEST_USER_PERMISSIONS:
    case VIX_E_GUEST_OPERATIONS_PROHIBITED:
    case VIX_E_ANON_GUEST_OPERATIONS_PROHIBITED:
    case VIX_E_ROOT_GUEST_OPERATIONS_PROHIBITED:
    case VIX_E_MISSING_ANON_GUEST_ACCOUNT:
    case VIX_E_CANNOT_AUTHENTICATE_WITH_GUEST:
      excType = VIXSecurityException;
      break;

    default:
      excType = VIXException;
      break;
  }

  assert (excType != NULL);
  return raiseVIXError(err, excType, NULL);
} /* raiseGenericVIXError */

#define SUPPRESS_EXCEPTION \
  suppressPythonExceptionIfAny(__FILE__, __LINE__)

static void suppressPythonExceptionIfAny(
    const char *file_name, const int line
  )
{
  if (PyErr_Occurred()) {
    fprintf(stderr, "pyvix ignoring exception\n");
    fprintf(stderr, "  on line %d\n", line);
    fprintf(stderr, "  of file %s:\n  ", file_name);
    PyErr_Print();
    /* PyErr_Print cleared the exception: */
    assert (!PyErr_Occurred());
  }
} /* suppress_exception_if_any */
