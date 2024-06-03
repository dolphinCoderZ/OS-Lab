#include "../include/conix/list.h"
#include "../include/conix/assert.h"

// ��ʼ������
void list_init(list_t *list) {
  list->head.prev = NULL;
  list->tail.next = NULL;
  list->head.next = &list->tail;
  list->tail.prev = &list->head;
}

// �� anchor ���ǰ������ node
void list_insert_before(list_node_t *anchor, list_node_t *node) {
  node->prev = anchor->prev;
  node->next = anchor;

  anchor->prev->next = node;
  anchor->prev = node;
}

// �� anchor ��������� node
void list_insert_after(list_node_t *anchor, list_node_t *node) {
  node->prev = anchor;
  node->next = anchor->next;

  anchor->next->prev = node;
  anchor->next = node;
}

// ���뵽ͷ����
void list_push(list_t *list, list_node_t *node) {
  assert(!list_search(list, node));
  list_insert_after(&list->head, node);
}

// �Ƴ�ͷ����Ľ��
list_node_t *list_pop(list_t *list) {
  assert(!list_empty(list));

  list_node_t *node = list->head.next;
  list_remove(node);

  return node;
}

// ���뵽β���ǰ
void list_pushback(list_t *list, list_node_t *node) {
  assert(!list_search(list, node));
  list_insert_before(&list->tail, node);
}

// �Ƴ�β���ǰ�Ľ��
list_node_t *list_popback(list_t *list) {
  assert(!list_empty(list));

  list_node_t *node = list->tail.prev;
  list_remove(node);

  return node;
}

// ���������н���Ƿ����
bool list_search(list_t *list, list_node_t *node) {
  list_node_t *next = list->head.next;
  while (next != &list->tail) {
    if (next == node)
      return true;
    next = next->next;
  }
  return false;
}

// ��������ɾ�����
void list_remove(list_node_t *node) {
  assert(node->prev != NULL);
  assert(node->next != NULL);

  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = NULL;
  node->prev = NULL;
}

// �ж������Ƿ�Ϊ��
bool list_empty(list_t *list) { return (list->head.next == &list->tail); }

// ���������
uint32 list_size(list_t *list) {
  list_node_t *next = list->head.next;
  uint32 size = 0;
  while (next != &list->tail) {
    size++;
    next = next->next;
  }
  return size;
}

// �����������
void list_insert_sort(list_t *list, list_node_t *node, int offset) {
  // �������ҵ���һ���ȵ�ǰ�ڵ� key �����Ľڵ㣬���в��뵽ǰ��
  list_node_t *anchor = &list->tail;
  int key = element_node_key(node, offset);
  for (list_node_t *ptr = list->head.next; ptr != &list->tail;
       ptr = ptr->next) {
    int compare = element_node_key(ptr, offset);
    if (compare > key) {
      anchor = ptr;
      break;
    }
  }

  assert(node->next == NULL);
  assert(node->prev == NULL);

  // ��������
  list_insert_before(anchor, node);
}