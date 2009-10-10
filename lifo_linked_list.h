/******************************************************************************
 * pyvix - LIFO Linked List Type and Other Tracker Support Code
 * Copyright 2006 David S. Rushby
 * Available under the MIT license (see docs/license.txt for details).
 *****************************************************************************/

#ifndef LIFO_LINKED_LIST_H
#define LIFO_LINKED_LIST_H

#define DEFINE_TRACKER_TYPES(ContainedType) \
  typedef struct _ ## ContainedType ## Tracker { \
    ContainedType *contained; \
    struct _ ## ContainedType ## Tracker *next; \
  } ContainedType ## Tracker; \
  \
  typedef status (* ContainedType ## TrackerMappedFunction) \
    (ContainedType ## Tracker *, ContainedType ## Tracker *);

#define LIFO_LINKED_LIST_DEFINE_CONS( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
    allocation_func \
  ) \
    static status _ ## ListType ## _cons(ListTypeQualified **list_slot, \
        ContainedTypeQualified *cont, ListTypeQualified *next \
      ) \
    { \
      /* Maintain a pointer to the previous occupant of the 'into' slot so we \
       * can restore it if the mem alloc fails. */ \
      ListTypeQualified *prev_occupant = *list_slot; \
      *list_slot = allocation_func(sizeof(ListType)); \
      if (*list_slot == NULL) { \
        *list_slot = prev_occupant; \
        return FAILED; \
      } \
      /* Initialize the two fields of the new node: */ \
      (*list_slot)->contained = cont; \
      (*list_slot)->next = next; \
      return SUCCEEDED; \
    }


#define LIFO_LINKED_LIST_DEFINE_ADD( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified \
  ) \
    static status ListType ## _add( \
        ListTypeQualified **list_slot, ContainedTypeQualified *cont \
      ) \
    { \
      assert (list_slot != NULL); \
      \
      if (*list_slot == NULL) { \
        if (_ ## ListType ## _cons(list_slot, cont, NULL) != SUCCEEDED) { \
          return FAILED; \
        } \
      } else { \
        ListTypeQualified *prev_head = *list_slot; \
        if (_ ## ListType ## _cons(list_slot, cont, prev_head) != SUCCEEDED) { \
          return FAILED; \
        } \
      } \
      \
      assert ((*list_slot)->contained == cont); \
      \
      return SUCCEEDED; \
    }

#define LIFO_LINKED_LIST_DEFINE_REMOVE( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
    deallocation_func \
  ) \
    static status ListType ## _remove( \
        ListTypeQualified **list_slot, ContainedTypeQualified *cont, \
        bool object_if_missing \
      ) \
    { \
      ListTypeQualified *nodeBack; \
      ListTypeQualified *nodeForward; \
      \
      /* Traverse the linked list looking for a node whose ->reader points to \
       * cont: */ \
      nodeBack = nodeForward = *list_slot; \
      while (nodeForward != NULL && nodeForward->contained != cont) { \
        nodeBack = nodeForward; \
        nodeForward = nodeForward->next; \
      } \
      if (nodeForward == NULL) { \
        if (!object_if_missing) { \
          return SUCCEEDED; \
        } else { \
          /* Note that this calls the Py_* API; GIL must be held. */ \
          raiseNonNumericVIXError(VIXInternalError, \
              # ListType "_remove: node was not in list" \
            ); \
          return FAILED; \
        } \
      } \
      \
      /* Unlink nodeForward: */ \
      if (nodeBack == nodeForward) { \
        /* We found the desired node in the first position of the linked \
         * list. */ \
        *list_slot = nodeForward->next; \
      } else { \
        nodeBack->next = nodeForward->next; \
      } \
      deallocation_func((ListType *) nodeForward); \
      \
      return SUCCEEDED; \
    }

#define LIFO_LINKED_LIST_DEFINE_RELEASE( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
    deallocation_func \
  ) \
    static status ListType ## _release(ListTypeQualified **list_slot) \
    { \
      ListTypeQualified *list; \
      assert (list_slot != NULL); \
      list = *list_slot; \
      if (list == NULL) { \
        /* It's already been released. */ \
        return SUCCEEDED; \
      } \
      \
      do { \
        ContainedTypeQualified *cont = list->contained; \
        assert (cont != NULL); \
        \
        /* Direct the contained object not to try to unlink itself upon \
         * closure, because that's the very thing we're doing! */ \
        if (ContainedType ## _untrack(cont, true) != SUCCEEDED) { \
          /* We weren't able to collect this node properly.  Make sure \
           * list_slot points to this node (which is currently the head of the \
           * linked list) so that the entire linked list isn't lost. */ \
          list_slot = &list; \
          return FAILED; \
        } else { \
          ListTypeQualified *next_list = list->next; \
          deallocation_func((ListType *) list); \
          list = next_list; \
        } \
      } while (list != NULL); \
      \
      *list_slot = NULL; \
      return SUCCEEDED; \
    }

#define LIFO_LINKED_LIST_DEFINE_TRAVERSE( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified \
  ) \
    static status ListType ## _traverse(ListTypeQualified *node_start, \
        ListType ## MappedFunction modifier \
      ) \
    { \
      ListTypeQualified *node_cur = node_start; \
      ListTypeQualified *node_prev = NULL; \
      \
      while (node_cur != NULL) { \
        if (modifier(node_prev, node_cur) != SUCCEEDED) { \
          goto fail; \
        } \
        node_prev = node_cur; \
        node_cur = node_cur->next; \
      } \
      \
      return SUCCEEDED; \
      \
      fail: \
        assert (PyErr_Occurred()); \
        return FAILED; \
    }

#define LIFO_LINKED_LIST_DEFINE_TRAVERSE_NOQUAL(ListType, ContainedType) \
  LIFO_LINKED_LIST_DEFINE_TRAVERSE( \
      ListType, ListType, ContainedType, ContainedType \
    )

#define LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
    allocation_func, deallocation_func \
  ) \
    LIFO_LINKED_LIST_DEFINE_CONS( \
        ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
        allocation_func \
      ) \
    LIFO_LINKED_LIST_DEFINE_ADD( \
        ListType, ListTypeQualified, ContainedType, ContainedTypeQualified \
      ) \
    LIFO_LINKED_LIST_DEFINE_REMOVE( \
        ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
        deallocation_func \
      ) \
    LIFO_LINKED_LIST_DEFINE_RELEASE( \
        ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
        deallocation_func \
      )

#define LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_PYALLOC( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified \
  ) \
    /* The GIL must be held during method calls to this linked list type. */ \
    LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_( \
        ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
        pyvix_main_malloc, pyvix_main_free \
      )

#define LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_PYALLOC_NOQUAL( \
    ListType, ContainedType \
  ) \
    LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_PYALLOC( \
        ListType, ListType, ContainedType, ContainedType \
      )

#define LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_SYSALLOC( \
    ListType, ListTypeQualified, ContainedType, ContainedTypeQualified \
  ) \
    /* The GIL does not need to be held during method calls to this linked \
     * list type. */ \
    LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_( \
        ListType, ListTypeQualified, ContainedType, ContainedTypeQualified, \
        pyvix_plain_malloc, pyvix_plain_free \
      )

#define LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_SYSALLOC_NOQUAL( \
    ListType, ContainedType \
  ) \
    LIFO_LINKED_LIST_DEFINE_BASIC_METHODS_SYSALLOC( \
        ListType, ListType, ContainedType, ContainedType \
      )

#endif /* not def LIFO_LINKED_LIST_H */
