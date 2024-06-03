#include "../include/conix/string.h"

char *strcpy(char *dest, const char *src) {
  while (*src != EOS) {
    *dest = *src;
    dest++;
    src++;
  }
  *dest = *src; // 拷贝最后的'\0'
  return dest;
}

// 末尾不自动添加'\0'，但src长度不足count时会填充'\0'
char *strncpy(char *dest, const char *src, size_t count) {
  char *ptr = dest;
  while (count--) {
    *ptr = *src;
    if (*src == EOS) {
      return dest;
    }
  }

  while (count--) {
    *ptr++ = EOS;
  }
  return dest;
}

char *strcat(char *dest, const char *src) {
  while (*dest != EOS) {
    dest++;
  }

  while (*src != EOS) {
    *dest = *src;
    dest++;
    src++;
  }
  *dest = *src;
  return dest;
}

// 不包含'\0'
size_t strlen(const char *str) {
  const char *p = str;
  while (*p != EOS) {
    p++;
  }
  return p - str;
}

int strcmp(const char *lhs, const char *rhs) {
  while (*lhs == *rhs) {
    if (*lhs == EOS) {
      return 0;
    }
    lhs++;
    rhs++;
  }
  return *lhs > *rhs ? 1 : -1;
}

// 查找第一个指定字符
char *strchr(const char *str, int ch) {
  char *ptr = (char *)str;
  while (*ptr != EOS) {
    if (*ptr == (char)ch) {
      return ptr;
    }
    ptr++;
  }
  return NULL;
}

// 查找最后一个指定字符
char *strrchr(const char *str, int ch) {
  char *last = NULL;
  char *ptr = (char *)str;

  while (*ptr != EOS) {
    if (*ptr == (char)ch) {
      last = ptr;
    }
    ptr++;
  }
  return last;
}

int memcmp(const void *lhs, const void *rhs, size_t count) {
  char *lptr = (char *)lhs;
  char *rptr = (char *)rhs;
  while (count--) {
    if (*lptr != *rptr) {
      return *lptr < *rptr ? -1 : 1;
    }
    lptr++;
    rptr++;
  }
  return 0;
}

void *memset(void *dest, int ch, size_t count) {
  char *ptr = dest;
  while (count--) {
    *ptr = (char)ch;
    ptr++;
  }
  return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
  char *ptr = dest;
  while (count--) {
    *ptr = *(char *)src;
    ptr++;
    src++;
  }
  return dest;
}

void *memchr(const void *ptr, int ch, size_t count) {
  char *p = (char *)ptr;
  while (count--) {
    if (*p == (char)ch) {
      return (void *)p;
    }
    p++;
  }
  return NULL;
}