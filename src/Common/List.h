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

struct LinkedListNode {
  struct LinkedListNode* next;
  struct LinkedListNode* prev;
};

/**
 * @brief Initiliaze empty linked list
 */
static inline void list_init(struct LinkedListNode* node) {
  node->next = node;
  node->prev = node;
}

/**
 * @brief Check if linked list is empty.
 * @return 1 if list is empty, 0 otherwise
 */
static inline int list_is_empty(struct LinkedListNode* node) {
  if (node->next == node) {
    assert(node->prev == node);
    return 1;
  }
  return 0;
}

/**
 * @brief Delete node from linked list, turning the node into empty list
 * @return Deleted node
 */
static inline struct LinkedListNode* list_take_node(struct LinkedListNode* node) {
  if (list_is_empty(node)) {
    return node;
  }
  struct LinkedListNode* prev = node->prev;
  struct LinkedListNode* next = node->next;
  prev->next = next;
  next->prev = prev;
  list_init(node);
  return node;
}

/**
 * @brief Insert new node after given node
 *
 * @warning New node should not already be in list
 */
static inline void list_insert_node(
    struct LinkedListNode* after,
    struct LinkedListNode* node
) {
  assert(after != node);
  struct LinkedListNode* before = after->next;
  assert(node != before);
  assert(before->prev == after);

  after->next = node;
  before->prev = node;
  node->prev = after;
  node->next = before;
}

/**
 * @brief Iterate over linked list
 *
 * @param name Name of iteration variable.
 * @param list Non-pointer root of linked list.
 *             Root is assumed to not contain actual data.
 */
#define LIST_FOREACH(name, list)\
  for (\
      struct LinkedListNode* node = (list).next;\
      node != &list;\
      node = node->next\
  )

/**
 * @brief Iterate over linked list with constant iteration variable
 *
 * @param name Name of iteration variable.
 * @param list Non-pointer root of linked list.
 *             Root is assumed to not contain actual data.
 */
#define LIST_CONST_FOREACH(name, list)\
  for (\
      const struct LinkedListNode* node = (list).next;\
      node != &list;\
      node = node->next\
  )

#endif /* List.h */
