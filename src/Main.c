#include <assert.h>
#include <stdio.h>

#include "Common/List.h"

typedef struct {
  LinkedListNode as_node;
  int value;
} IntListNode;

static void int_list_print(const LinkedList* list) {
  LIST_CONST_FOREACH(node, *list) {
    const IntListNode* int_node = (const IntListNode*) node;
    printf("%d ", int_node->value);
  }
  puts("");
}

int main()
{
  LinkedList int_list;
  list_init(&int_list);

  IntListNode nodes[5];
  for (size_t i = 0; i < 5; ++i) {
    list_node_init(&nodes[i].as_node);
    nodes[i].value = i;
    list_push_back(&int_list, &nodes[i].as_node);
  }
  int_list_print(&int_list);

  return 0;
}
