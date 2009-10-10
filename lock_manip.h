/******************************************************************************
 * pyvix - Lock Manipulation Code
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

#ifndef _LOCK_MANIP_H
#define _LOCK_MANIP_H

#define ENTER_PYTHON Py_END_ALLOW_THREADS
#define LEAVE_PYTHON Py_BEGIN_ALLOW_THREADS

#define ENTER_PYTHON_WITHOUT_CODE_BLOCK(gstate) \
  gstate = PyGILState_Ensure();

#define LEAVE_PYTHON_WITHOUT_CODE_BLOCK(gstate) \
  PyGILState_Release(gstate);

#endif /* not def _LOCK_MANIP_H */
