#ifndef CONIX_STAT_H
#define CONIX_STAT_H

#include "types.h"

// �ļ�����
#define IFMT 00170000  // �ļ����ͣ�8 ���Ʊ�ʾ��
#define IFREG 0100000  // �����ļ�
#define IFBLK 0060000  // �����⣨�豸���ļ�������� dev/fd0
#define IFDIR 0040000  // Ŀ¼�ļ�
#define IFCHR 0020000  // �ַ��豸�ļ�
#define IFIFO 0010000  // FIFO �����ļ�
#define IFLNK 0120000  // ��������
#define IFSOCK 0140000 // �׽���

// �ļ�����λ��
// ISUID ���ڲ����ļ��� set-user-ID ��־�Ƿ���λ
// ���ñ�־��λ����ִ�и��ļ�ʱ��
// ���̵���Ч�û� ID ��������Ϊ���ļ��������û� ID
// ISGID ��������� ID ������ͬ����
#define ISUID 0004000 // ִ��ʱ�����û� ID��set-user-ID��
#define ISGID 0002000 // ִ��ʱ������ ID��set-group-ID��

// ����Ϊ��λ�����ʾ���ļ��û�û��ɾ��Ȩ��
#define ISVTX 0001000 // ����Ŀ¼������ɾ����־

#define ISREG(m) (((m) & IFMT) == IFREG)   // �ǳ����ļ�
#define ISDIR(m) (((m) & IFMT) == IFDIR)   // ��Ŀ¼�ļ�
#define ISCHR(m) (((m) & IFMT) == IFCHR)   // ���ַ��豸�ļ�
#define ISBLK(m) (((m) & IFMT) == IFBLK)   // �ǿ��豸�ļ�
#define ISFIFO(m) (((m) & IFMT) == IFIFO)  // �� FIFO �����ļ�
#define ISLNK(m) (((m) & IFMT) == IFLNK)   // �Ƿ��������ļ�
#define ISSOCK(m) (((m) & IFMT) == IFSOCK) // ���׽����ļ�
#define ISFILE(m) ISREG(m)                 // ��ֱ�۵�һ����

// �ļ�����Ȩ��
#define IRWXU 00700 // �������Զ���д��ִ��/����
#define IRUSR 00400 // ���������
#define IWUSR 00200 // ����д���
#define IXUSR 00100 // ����ִ��/�������

#define IRWXG 00070 // ���Ա���Զ���д��ִ��/����
#define IRGRP 00040 // ���Ա�����
#define IWGRP 00020 // ���Աд���
#define IXGRP 00010 // ���Աִ��/�������

#define IRWXO 00007 // �����˶���д��ִ��/�������
#define IROTH 00004 // �����˶����
#define IWOTH 00002 // ������д���
#define IXOTH 00001 // ������ִ��/�������

typedef struct stat_t {
  dev_t dev;    // �����ļ����豸��
  idx_t nr;     // �ļ� i �ڵ��
  uint16 mode;  // �ļ����ͺ�����
  uint8 nlinks; // ָ���ļ���������
  uint16 uid;   // �ļ����û�(��ʶ)��
  uint8 gid;    // �ļ������
  dev_t rdev;   // �豸��(����ļ���������ַ��ļ�����ļ�)
  size_t size;  // �ļ���С���ֽ�����������ļ��ǳ����ļ���
  time_t atime; // �ϴΣ���󣩷���ʱ��
  time_t mtime; // ����޸�ʱ��
  time_t ctime; // ���ڵ��޸�ʱ��
} stat_t;

#endif