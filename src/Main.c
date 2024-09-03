#include <assert.h>
#include <stdio.h>

#include "Common/List.h"

struct IntList {
  struct LinkedListNode as_list;
  int value;
};

static void int_list_print(const struct LinkedListNode* list) {
  LIST_CONST_FOREACH(node, *list) {
    const struct IntList* int_node = (const struct IntList*) node;
    printf("%d ", int_node->value);
  }
  puts("");
}

int main()
{
  struct LinkedListNode int_list;
  list_init(&int_list);

  struct IntList nodes[5];
  for (size_t i = 0; i < 5; ++i) {
    nodes[i].value = i;
    list_insert_node(int_list.prev, &nodes[i].as_list);
  }
  int_list_print(&int_list);

  struct IntList* two = (struct IntList*) int_list.next->next->next;
  assert(two->value == 2);

  list_take_node(&two->as_list);
  int_list_print(&int_list);

  list_insert_node(&int_list, &two->as_list);
  int_list_print(&int_list);


  return 0;
}
