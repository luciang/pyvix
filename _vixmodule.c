/******************************************************************************
 * pyvix - Implementation of Main Module Infrastructure
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

#include "_vixmodule.h"

#include "error_handling.c"
#include "util.c"

#include "stateful_handle_wrapper.c"
#include "callback_accumulator.c"

#include "snapshot.c"
#include "vm.c"
#include "host.c"

#include "constants.c"

static PyMethodDef vix_globalMethods[] = {
    { "initSupport_Constants",
        initSupport_Constants,
        METH_VARARGS
      },
    {NULL, NULL, 0, NULL}
  };

DL_EXPORT(void)
init_vixmodule(void) {
  PyObject *m;

  m = Py_InitModule("_vixmodule", vix_globalMethods);
  if (m == NULL) { goto fail; }

  if (initSupport_errorHandling(m) != SUCCEEDED) { goto fail; }

  #define _INIT_C_TYPE_AND_SYS(type_name) { \
    status status = initSupport_ ## type_name(); \
    if (status == SUCCEEDED) { \
      /* Expose the class in the namespace of the _vixmodule module. \
       * PyModule_AddObject steals a ref, so the artificial INCREF prevents \
       * a client programmer from messing up the refcount with a statement \
       * like 'del vix._vixmodule.ClassName'. */ \
      Py_INCREF(&type_name ## Type); \
      status = PyModule_AddObject(m, \
          #type_name, (PyObject *) &type_name ## Type \
        ); \
    } \
    if (status != SUCCEEDED) { goto fail; } \
  }

  _INIT_C_TYPE_AND_SYS(Host);
  _INIT_C_TYPE_AND_SYS(VM);
  _INIT_C_TYPE_AND_SYS(Snapshot);

  return;
  fail:
    /* Python's module loader doesn't really give us any error handling
     * options: */
    return;
} /* init_vixmodule */
