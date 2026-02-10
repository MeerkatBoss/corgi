/**
 * @file List.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief Double-linked list data structure
 *
 * @version 0.1
 * @date 2024-09-03
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __COMMON_LIST_H
#define __COMMON_LIST_H

#include <assert.h>
#include <stddef.h>

typedef struct LinkedListNode_ {
  struct LinkedListNode_* next;
  struct LinkedListNode_* prev;
} LinkedListNode;

typedef struct {
  LinkedListNode root;
} LinkedList;

static inline void list_node_init(LinkedListNode* node) {
  node->next = NULL;
  node->prev = NULL;
}

static inline int list_node_is_null(LinkedListNode* node) {
  assert( (node->prev == NULL) == (node->next == NULL) );
  return node->prev == NULL;
}

static inline void list_init(LinkedList* list) {
  list->root.next = &list->root;
  list->root.prev = &list->root;
}

static inline int list_is_empty(LinkedList* list) {
  if (list->root.next == &list->root) {
    assert(list->root.prev == &list->root);
    return 1;
  }
  return 0;
}

/**
 * @brief Delete node from linked list
 *
 * @return Deleted node
 *
 * @error Root node of list cannot be taken
 */
static inline LinkedListNode* list_take_node(LinkedListNode* node) {
  LinkedListNode* prev = node->prev;
  LinkedListNode* next = node->next;
  /* Only root node can be self-referential */
  assert(prev != node);
  assert(next != node);

  prev->next = next;
  next->prev = prev;
  list_node_init(node);
  return node;
}

/**
 * @brief Insert new node after given node
 *
 * @warning New node should be null
 */
static inline void list_insert_node(
    LinkedListNode* after,
    LinkedListNode* node
) {
  assert(list_node_is_null(node));
  LinkedListNode* before = after->next;

  after->next = node;
  before->prev = node;
  node->prev = after;
  node->next = before;
}

/**
 * @brief Prepend node to list
 * 
 * @warning New node should not already be in list
 */
static inline void list_push_front(
    LinkedList* list,
    LinkedListNode* node
) {
  list_insert_node(&list->root, node);
}

/**
 * @brief Append node to list
 * 
 * @warning New node should not already be in list
 */
static inline void list_push_back(
    LinkedList* list,
    LinkedListNode* node
) {
  list_insert_node(list->root.prev, node); 
}

/**
 * @brief Remove first node from list
 *
 * @return Removed node or NULL if list is empty
 */
static inline LinkedListNode* list_pop_front(LinkedList* list) {
  if (list_is_empty(list)) {
    return NULL;
  }
  LinkedListNode* node = list_take_node(list->root.next);

  /* Placate clang-tidy: node is guaranteed to not be in list anymore */
  assert(list->root.next != node);
  assert(list->root.prev != node);

  return node;
}

/**
 * @brief Remove last node from list
 *
 * @return Removed node or NULL if list is empty
 */
static inline LinkedListNode* list_pop_back(LinkedList* list) {
  if (list_is_empty(list)) {
    return NULL;
  }
  LinkedListNode* node = list_take_node(list->root.prev);

  /* Placate clang-tidy: node is guaranteed to not be in list anymore */
  assert(list->root.next != node);
  assert(list->root.prev != node);

  return node;
}

/**
 * @brief Iterate over linked list
 *
 * @param name Name of iteration variable.
 * @param list Linked list
 */
#define LIST_FOREACH(name, list)\
  for (\
      LinkedListNode* name = (list).root.next;\
      name != &(list).root;\
      name = name->next\
  )

/**
 * @brief Iterate over linked list with constant iteration variable
 *
 * @param name Name of iteration variable.
 * @param list Linked list
 */
#define LIST_CONST_FOREACH(name, list)\
  for (\
      const LinkedListNode* name = (list).root.next;\
      name != &(list).root;\
      name = name->next\
  )

#endif /* List.h */
