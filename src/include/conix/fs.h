#ifndef CONIX_FS_H
#define CONIX_FS_H

#include "list.h"
#include "types.h"

#define BLOCK_SIZE 1024
#define SECTOR_SIZE 512

#define MINIX1_MAGIC 0x137f
#define NAME_LEN 14

#define IMAP_NR 8
#define ZMAP_NR 8

#define BLOCK_BITS (BLOCK_SIZE * 8)                      // ��λͼ��С
#define BLOCK_INODES (BLOCK_SIZE / sizeof(inode_desc_t)) // ��inode����
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(dentry_t))   // ��dentry����
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(uint16))      // ����������

#define DIRECT_BLOCK (7)
#define INDIRECT1_BLOCK BLOCK_INDEXES
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK)
#define TOTAL_BLOCK (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK)

#define SEPARTOR1 '/'
#define SEPARTOR2 '\\'
#define IS_SEPARTOR(c) (c == SEPARTOR1 || c == SEPARTOR2)

enum file_flag {
  O_RDONLY = 00,      // ֻ����ʽ
  O_WRONLY = 01,      // ֻд��ʽ
  O_RDWR = 02,        // ��д��ʽ
  O_ACCMODE = 03,     // �ļ�����ģʽ������
  O_CREAT = 00100,    // ����ļ������ھʹ���
  O_EXCL = 00200,     // ��ռʹ���ļ���־
  O_NOCTTY = 00400,   // ����������ն�
  O_TRUNC = 01000,    // ���ļ��Ѵ�������д�������򳤶Ƚ�Ϊ 0
  O_APPEND = 02000,   // ����ӷ�ʽ�򿪣��ļ�ָ����Ϊ�ļ�β
  O_NONBLOCK = 04000, // ��������ʽ�򿪺Ͳ����ļ�
};

typedef struct inode_desc_t {
  uint16 mode; // �ļ����ͺ�����
  uint16 uid;  // �û�id
  uint32 size;
  uint32 mtime;
  uint8 gid;
  uint8 nlinks;
  uint16 zone[9]; // ֱ��0-6�����7��˫�ؼ��8 �߼����
} inode_desc_t;

typedef struct inode_t {
  inode_desc_t *desc;
  struct buffer_t *buf;
  dev_t dev;
  idx_t nr;     // i �ڵ��
  uint32 count; // ���ü���
  time_t atime; // ����ʱ��
  time_t ctime; // �޸�ʱ��
  list_node_t node;
  dev_t mount; // ���ص��豸
} inode_t;

typedef struct super_desc_t {
  uint16 inodes;        // inode�ڵ���
  uint16 zones;         // �߼�����
  uint16 imap_blocks;   // i�ڵ�λͼ��ռ�õ����ݿ���
  uint16 zmap_blocks;   // �߼���λͼ��ռ�õ����ݿ���
  uint16 firstdatazone; // ��һ�������߼����
  uint16 log_zone_size;
  uint32 max_size;
  uint16 magic;
} super_desc_t;

typedef struct super_block_t {
  super_desc_t *desc;
  struct buffer_t *buf;
  struct buffer_t *imap[IMAP_NR];
  struct buffer_t *zmap[ZMAP_NR];
  dev_t dev;
  list_t inode_list; // �򿪵�inode����
  inode_t *iroot;    // ��Ŀ¼inode
  inode_t *imount;
} super_block_t;

typedef struct dentry_t {
  uint16 nr;
  char name[NAME_LEN];
} dentry_t;

typedef struct file_t {
  inode_t *inode;
  uint32 count;
  off_t offset;
  int flags;
  int mode;
} file_t;

typedef enum whence_t {
  SEEK_SET = 1, // ֱ������ƫ��
  SEEK_CUR,     // ��ǰλ��ƫ��
  SEEK_END      // ����λ��ƫ��
} whence_t;

super_block_t *get_super(dev_t dev);
super_block_t *read_super(dev_t dev);

// ����λͼ
idx_t balloc(dev_t dev);
void bfree(dev_t dev, idx_t idx);
idx_t ialloc(dev_t dev);
void ifree(dev_t dev, idx_t idx);
idx_t bmap(inode_t *inode, idx_t block, bool create);

inode_t *get_root_inode(); // ��Ŀ¼inode
inode_t *iget(dev_t dev, idx_t nr);
void iput(inode_t *inode);

inode_t *named(char *pathname, char **next);
inode_t *namei(char *pathname);

inode_t *inode_open(char *pathname, int flag, int mode);
int inode_read(inode_t *inode, char *buf, uint32 len, off_t offset);
int inode_write(inode_t *inode, char *buf, uint32 len, off_t offset);
void inode_truncate(inode_t *inode);

#endif