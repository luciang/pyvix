/******************************************************************************
 * pyvix - Memory Allocation, Reallocation, and Deallocation Families
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

#ifndef MEMORY_SYSTEMS_H
#define MEMORY_SYSTEMS_H

/***************************     PLAIN     ***********************************/

/* pyvix_plain_* is PyVix's simplest series of memory handlers.
 * Typically, these will simply be aliases for the standard C memory handlers.
 *
 * pyvix_plain_* should NEVER resolve to pymalloc's memory handlers, nor to any
 * other memory allocator that's expected to be incompatible with "generic"
 * memory allocated "by some third party".
 *
 * Also, unlike pyvix_main_*, this series must be threadsafe.                */
#define pyvix_plain_malloc          malloc
#define pyvix_plain_realloc         realloc
#define pyvix_plain_free            free


/***************************     MAIN      ***********************************/

/* Series for pymalloc, the specialized Python-oriented memory handler that
 * became standard in Python 2.3.
 *
 * Unless there's a specific reason not to (as noted elsewhere in this file,
 * including in the WARNING just below), any [|de|re]allocation of raw memory
 * should use this series.
 *
 * WARNING:
 *   Members of the pyvix_main_* series must only be called when the GIL is
 * held, since they rely on pymalloc, which assumes the GIL is held.         */
#define pyvix_main_malloc           PyObject_Malloc
#define pyvix_main_realloc          PyObject_Realloc
#define pyvix_main_free             PyObject_Free

/***************************    VIX BUFFER   *********************************/

/* The VIX API requires that buffers it allocates and returns to the client be
 * offers a free function freed with Vix_FreeBuffer. */
#define pyvix_vix_buffer_free       Vix_FreeBuffer

#endif /* MEMORY_SYSTEMS_H */
