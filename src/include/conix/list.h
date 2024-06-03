#ifndef CONIX_LIST_H
#define CONIX_LIST_H

#include "types.h"

#define element_offset(type, member) (uint32)(&((type *)0)->member)
#define element_entry(type, member, ptr)                                       \
  (type *)((uint32)ptr - element_offset(type, member))
#define element_node_offset(type, node, key)                                   \
  ((int)(&((type *)0)->key) - (int)(&((type *)0)->node))
#define element_node_key(node, offset) *(int *)((int)node + offset)

// ������
typedef struct list_node_t {
  struct list_node_t *prev; // ��һ�����
  struct list_node_t *next; // ǰһ�����
} list_node_t;

// ����
typedef struct list_t {
  list_node_t head; // ͷ���
  list_node_t tail; // β���
} list_t;

// ��ʼ������
void list_init(list_t *list);

// �� anchor ���ǰ������ node
void list_insert_before(list_node_t *anchor, list_node_t *node);

// �� anchor ��������� node
void list_insert_after(list_node_t *anchor, list_node_t *node);

// ���뵽ͷ����
void list_push(list_t *list, list_node_t *node);

// �Ƴ�ͷ����Ľ��
list_node_t *list_pop(list_t *list);

// ���뵽β���ǰ
void list_pushback(list_t *list, list_node_t *node);

// �Ƴ�β���ǰ�Ľ��
list_node_t *list_popback(list_t *list);

// ���������н���Ƿ����
bool list_search(list_t *list, list_node_t *node);

// ��������ɾ�����
void list_remove(list_node_t *node);

// �ж������Ƿ�Ϊ��
bool list_empty(list_t *list);

// ���������
uint32 list_size(list_t *list);

// �����������
void list_insert_sort(list_t *list, list_node_t *node, int offset);

#endif