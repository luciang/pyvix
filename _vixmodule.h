/******************************************************************************
 * pyvix - Main Header File
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

#ifndef VIXMODULE_H
#define VIXMODULE_H

/* Define PY_SSIZE_T_CLEAN to make PyArg_ParseTuple's "s#" and "t#" codes
 * output Py_ssize_t rather than int when using Python 2.5+.
 * This definition must appear before #include "Python.h". */
#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "vix.h"

/************************ PYTHON VERSION HOOP-JUMPING ************************/
#if (PY_MAJOR_VERSION >= 2)
  #if (PY_MINOR_VERSION >= 4)
    #define PYTHON_2_4_OR_LATER
  #endif
  #if (PY_MINOR_VERSION >= 5)
    #define PYTHON_2_5_OR_LATER
  #endif
#endif

/* Provide aliases for Py_ssize_t-related stuff for versions of Python prior
 * to 2.5: */
#ifndef PYTHON_2_5_OR_LATER
  typedef int Py_ssize_t;
  #define PY_SSIZE_T_MAX INT_MAX
  #define PY_SSIZE_T_MIN INT_MIN

  #define Py_ssize_t_STRING_FORMAT "%d"
  #define Py_ssize_t_EXTRACTION_CODE "i"

  #define PyInt_FromSsize_t PyInt_FromLong
  #define PyInt_AsSsize_t   PyInt_AsLong
#else
  #define Py_ssize_t_STRING_FORMAT "%zd"
  #define Py_ssize_t_EXTRACTION_CODE "n"
#endif
#define SIZE_T_TO_PYTHON_SIZE(x) ((Py_ssize_t) (x))
#define PYTHON_SIZE_TO_SIZE_T(x) ((size_t) (x))

/* As of Python 2.5a1, a preprocessor statement such as
 *   #if PY_SSIZE_T_MAX > LONG_MAX
 * is rejected by the compiler, but assigning PY_SSIZE_T_MAX to a constant,
 * then referring to that constant, is accepted. */
const Py_ssize_t PY_SSIZE_T_MAX__CIRCUMVENT_COMPILER_COMPLAINT = PY_SSIZE_T_MAX;


/* Python's long type, which has unlimited range, is computationally
 * expensive relative to Python's int type, which wraps a native long.  So:
 * 1. On platforms where C's long is 64 bits or larger,
 *    PythonIntOrLongFrom64BitValue unconditionally creates a Python int.
 * 2. On platforms where C's long is smaller than 64 bits,
 *    PythonIntOrLongFrom64BitValue tests the value at runtime to see if it's
 *    within the range of a Python int; if so, it creates a Python int rather
 *    than a Python long.  It is expected that in most contexts, the cost of
 *    the test will be less than the cost of using a Python long. */
#if defined(_MSC_VER) || defined(__BORLANDC__)
  /* MSVC 6 and BCPP 5.5 won't accept the LL suffix. */
  #define NativeLongIsAtLeast64Bits \
    (LONG_MAX >= 9223372036854775807i64 && LONG_MIN <= (-9223372036854775807i64 - 1))
#else
  #define NativeLongIsAtLeast64Bits \
    (LONG_MAX >= 9223372036854775807LL  && LONG_MIN <= (-9223372036854775807LL  - 1))
#endif

#if NativeLongIsAtLeast64Bits
  #define PythonIntOrLongFrom64BitValue(x) PyInt_FromLong((long) x)
#else
  #define PythonIntOrLongFrom64BitValue(x) \
    ( ((x) < LONG_MIN || (x) > LONG_MAX) ? PyLong_FromLongLong(x) : PyInt_FromLong((long) (x)) )
#endif

/***************************** CONVENIENCE DEFS ******************************/
const int32 NO_TIMEOUT = -1;

/******************************** TYPE DEFS **********************************/
#define VixHandle_EXTRACTION_CODE "i"
#define VixHandle_FUNCTION_CALL_CODE VixHandle_EXTRACTION_CODE

typedef enum {
  SUCCEEDED = 0,
  FAILED = -1
} status;

typedef enum {
  false = 0,
  true = 1
} bool;

/******************************* CLASS DEFS **********************************/

/* Supporting code used by some of the class defs: */
#include "memory_systems.h"
#include "lock_manip.h"
#include "lifo_linked_list.h"

/* StatefulHandleWrapper acts as a sort of "unofficial superclass" for all
 * Python types that primarily wrap VixHandles. */
typedef enum {
  STATE_CREATED = 0,
  STATE_OPEN    = 1,
  STATE_CLOSED  = 2
} GenericState;

#define StatefulHandleWrapper_HEAD \
  PyObject_HEAD /* Python API - infrastructural macro. */ \
  GenericState state; \
  VixHandle handle;

/* definition of "super-type" StatefulHandleWrapper: */
typedef struct _StatefulHandleWrapper {
  StatefulHandleWrapper_HEAD
} StatefulHandleWrapper;


/* Host class: */
typedef struct _Host {
  StatefulHandleWrapper_HEAD

  struct _VMTracker *openVMs;
} Host;
extern PyTypeObject HostType;


/* VM class: */
typedef struct _VM {
  StatefulHandleWrapper_HEAD

  Host *host;
  struct _SnapshotTracker *openSnapshots;
} VM;
extern PyTypeObject VMType;
DEFINE_TRACKER_TYPES(VM)


/* Snapshot class: */
typedef struct _Snapshot {
  StatefulHandleWrapper_HEAD

  VM *vm;
} Snapshot;
extern PyTypeObject SnapshotType;
DEFINE_TRACKER_TYPES(Snapshot)


/* VixCallbackAccumulator is designed to make the C code in pyvix that handles
 * callbacks from VIX more future-proof, by providing a place to put as-yet-
 * unanticipated fields. */
typedef struct {
  PyObject *target;
} VixCallbackAccumulator;

#endif /* ndef VIXMODULE_H */
