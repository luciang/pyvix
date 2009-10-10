/******************************************************************************
 * pyvix - Make the Essential VIX Constants Available to Python
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

/* SIC is just a shortcut for entering integer constants into dict d: */
#define SIC(name) \
  py_const = PyInt_FromLong(name); \
  if (py_const == NULL) { goto fail; } \
  res = PyDict_SetItemString(d, #name, py_const); \
  Py_DECREF(py_const); \
  if (res != 0) { goto fail; }

static PyObject *initSupport_Constants(PyObject *self, PyObject *args) {
  int res = -1;
  PyObject *py_const = NULL;

  PyObject *d;
  if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &d)) { return NULL; }

 /****************************************************************************/
 /* Custom constants: BEGIN                                                  */
 /****************************************************************************/
  SIC(NO_TIMEOUT);
 /****************************************************************************/
 /* Custom constants: END                                                    */
 /****************************************************************************/

 /****************************************************************************/
 /* VIX error codes: BEGIN                                                   */
 /****************************************************************************/
  SIC(VIX_OK);
  /* General errors */
  SIC(VIX_E_FAIL);
  SIC(VIX_E_OUT_OF_MEMORY);
  SIC(VIX_E_INVALID_ARG);
  SIC(VIX_E_FILE_NOT_FOUND);
  SIC(VIX_E_OBJECT_IS_BUSY);
  SIC(VIX_E_NOT_SUPPORTED);
  SIC(VIX_E_FILE_ERROR);
  SIC(VIX_E_DISK_FULL);
  SIC(VIX_E_INCORRECT_FILE_TYPE);
  SIC(VIX_E_CANCELLED);
  SIC(VIX_E_FILE_READ_ONLY);
  SIC(VIX_E_FILE_ALREADY_EXISTS);
  SIC(VIX_E_FILE_ACCESS_ERROR);
  SIC(VIX_E_REQUIRES_LARGE_FILES);
  SIC(VIX_E_FILE_ALREADY_LOCKED);
  /* Handle Errors */
  SIC(VIX_E_INVALID_HANDLE);
  SIC(VIX_E_NOT_SUPPORTED_ON_HANDLE_TYPE);
  SIC(VIX_E_TOO_MANY_HANDLES);
  /* XML errors */
  SIC(VIX_E_NOT_FOUND);
  SIC(VIX_E_TYPE_MISMATCH);
  SIC(VIX_E_INVALID_XML);
  /* VM Control Errors */
  SIC(VIX_E_TIMEOUT_WAITING_FOR_TOOLS);
  SIC(VIX_E_UNRECOGNIZED_COMMAND);
  SIC(VIX_E_OP_NOT_SUPPORTED_ON_GUEST);
  SIC(VIX_E_PROGRAM_NOT_STARTED);
  SIC(VIX_E_VM_NOT_RUNNING);
  SIC(VIX_E_VM_IS_RUNNING);
  SIC(VIX_E_CANNOT_CONNECT_TO_VM);
  SIC(VIX_E_POWEROP_SCRIPTS_NOT_AVAILABLE);
  SIC(VIX_E_NO_GUEST_OS_INSTALLED);
  SIC(VIX_E_VM_INSUFFICIENT_HOST_MEMORY);
  SIC(VIX_E_SUSPEND_ERROR);
  SIC(VIX_E_VM_NOT_ENOUGH_CPUS);
  SIC(VIX_E_HOST_USER_PERMISSIONS);
  SIC(VIX_E_GUEST_USER_PERMISSIONS);
  SIC(VIX_E_TOOLS_NOT_RUNNING);
  SIC(VIX_E_GUEST_OPERATIONS_PROHIBITED_ON_HOST);
  SIC(VIX_E_GUEST_OPERATIONS_PROHIBITED_ON_VM);
  SIC(VIX_E_ANON_GUEST_OPERATIONS_PROHIBITED_ON_HOST);
  SIC(VIX_E_ANON_GUEST_OPERATIONS_PROHIBITED_ON_VM);
  SIC(VIX_E_ROOT_GUEST_OPERATIONS_PROHIBITED_ON_HOST);
  SIC(VIX_E_ROOT_GUEST_OPERATIONS_PROHIBITED_ON_VM);
  SIC(VIX_E_MISSING_ANON_GUEST_ACCOUNT);
  SIC(VIX_E_CANNOT_AUTHENTICATE_WITH_GUEST);
  SIC(VIX_E_UNRECOGNIZED_COMMAND_IN_GUEST);
  /* VM Errors */
  SIC(VIX_E_VM_NOT_FOUND);
  SIC(VIX_E_NOT_SUPPORTED_FOR_VM_VERSION);
  SIC(VIX_E_CANNOT_READ_VM_CONFIG);
  SIC(VIX_E_TEMPLATE_VM);
  SIC(VIX_E_VM_ALREADY_LOADED);
  /* Property Errors */
  SIC(VIX_E_UNRECOGNIZED_PROPERTY);
  SIC(VIX_E_INVALID_PROPERTY_VALUE);
  SIC(VIX_E_READ_ONLY_PROPERTY);
  SIC(VIX_E_MISSING_REQUIRED_PROPERTY);
  /* Completion Errors */
  SIC(VIX_E_BAD_VM_INDEX);

 /****************************************************************************/
 /* VIX error codes: END                                                     */
 /****************************************************************************/

 /****************************************************************************/
 /* VixPropertyID constants: BEGIN                                           */
 /****************************************************************************/
  SIC(VIX_PROPERTY_NONE);

  /* VIX_HANDLETYPE_VM properties: */
  SIC(VIX_PROPERTY_VM_NUM_VCPUS);
  SIC(VIX_PROPERTY_VM_VMX_PATHNAME);
  SIC(VIX_PROPERTY_VM_VMTEAM_PATHNAME);
  SIC(VIX_PROPERTY_VM_MEMORY_SIZE);
  SIC(VIX_PROPERTY_VM_IN_VMTEAM);
  SIC(VIX_PROPERTY_VM_POWER_STATE);
  SIC(VIX_PROPERTY_VM_TOOLS_STATE);
  SIC(VIX_PROPERTY_VM_IS_RUNNING);

  /* JOB RESULT properties (not exposed to the client programmer because
   * they're meant for internal use).
  VIX_PROPERTY_JOB_RESULT_ERROR_CODE
  VIX_PROPERTY_JOB_RESULT_VM_IN_GROUP
  VIX_PROPERTY_JOB_RESULT_USER_MESSAGE
  VIX_PROPERTY_JOB_RESULT_LINE_NUM
  VIX_PROPERTY_JOB_RESULT_EXIT_CODE
  VIX_PROPERTY_JOB_RESULT_COMMAND_OUTPUT
  VIX_PROPERTY_JOB_RESULT_HANDLE
  VIX_PROPERTY_JOB_RESULT_FOUND_ITEM_NAME
  VIX_PROPERTY_JOB_RESULT_FOUND_ITEM_DESCRIPTION
  */

  /* EVENT properties (not exposed to the client programmer because they're
   * meant for internal use).
  VIX_PROPERTY_FOUND_ITEM_LOCATION
  */
 /****************************************************************************/
 /* VixPropertyID constants: END                                             */
 /****************************************************************************/

 /****************************************************************************/
 /* VixVMPowerOpOptions constants: BEGIN                                     */
 /****************************************************************************/
  SIC(VIX_VMPOWEROP_NORMAL);
 /****************************************************************************/
 /* VixVMPowerOpOptions constants: END                                       */
 /****************************************************************************/

 /****************************************************************************/
 /* VixPowerState constants: BEGIN                                           */
 /****************************************************************************/
  SIC(VIX_POWERSTATE_POWERING_OFF);
  SIC(VIX_POWERSTATE_POWERED_OFF);
  SIC(VIX_POWERSTATE_POWERING_ON);
  SIC(VIX_POWERSTATE_POWERED_ON);
  SIC(VIX_POWERSTATE_SUSPENDING);
  SIC(VIX_POWERSTATE_SUSPENDED);
  SIC(VIX_POWERSTATE_TOOLS_RUNNING);
  SIC(VIX_POWERSTATE_RESETTING);
  SIC(VIX_POWERSTATE_BLOCKED_ON_MSG);
 /****************************************************************************/
 /* VixPowerState constants: END                                             */
 /****************************************************************************/

 /****************************************************************************/
 /* VixToolsState constants: BEGIN                                           */
 /****************************************************************************/
  SIC(VIX_TOOLSSTATE_UNKNOWN);
  SIC(VIX_TOOLSSTATE_RUNNING);
  SIC(VIX_TOOLSSTATE_NOT_INSTALLED);
 /****************************************************************************/
 /* VixToolsState constants: END                                             */
 /****************************************************************************/

 /****************************************************************************/
 /* VixRunProgramOptions constants: BEGIN                                    */
 /****************************************************************************/
  SIC(VIX_RUNPROGRAM_RETURN_IMMEDIATELY);
 /****************************************************************************/
 /* VixRunProgramOptions constants: END                                      */
 /****************************************************************************/

  Py_RETURN_NONE;
  fail:
    assert (PyErr_Occurred());
    return NULL;
} /* initSupport_Constants */
