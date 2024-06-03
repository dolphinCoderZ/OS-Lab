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

#define BLOCK_BITS (BLOCK_SIZE * 8)                      // 块位图大小
#define BLOCK_INODES (BLOCK_SIZE / sizeof(inode_desc_t)) // 块inode数量
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(dentry_t))   // 块dentry数量
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(uint16))      // 块索引数量

#define DIRECT_BLOCK (7)
#define INDIRECT1_BLOCK BLOCK_INDEXES
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK)
#define TOTAL_BLOCK (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK)

#define SEPARTOR1 '/'
#define SEPARTOR2 '\\'
#define IS_SEPARTOR(c) (c == SEPARTOR1 || c == SEPARTOR2)

enum file_flag {
  O_RDONLY = 00,      // 只读方式
  O_WRONLY = 01,      // 只写方式
  O_RDWR = 02,        // 读写方式
  O_ACCMODE = 03,     // 文件访问模式屏蔽码
  O_CREAT = 00100,    // 如果文件不存在就创建
  O_EXCL = 00200,     // 独占使用文件标志
  O_NOCTTY = 00400,   // 不分配控制终端
  O_TRUNC = 01000,    // 若文件已存在且是写操作，则长度截为 0
  O_APPEND = 02000,   // 以添加方式打开，文件指针置为文件尾
  O_NONBLOCK = 04000, // 非阻塞方式打开和操作文件
};

typedef struct inode_desc_t {
  uint16 mode; // 文件类型和属性
  uint16 uid;  // 用户id
  uint32 size;
  uint32 mtime;
  uint8 gid;
  uint8 nlinks;
  uint16 zone[9]; // 直接0-6、间接7、双重间接8 逻辑块号
} inode_desc_t;

typedef struct inode_t {
  inode_desc_t *desc;
  struct buffer_t *buf;
  dev_t dev;
  idx_t nr;     // i 节点号
  uint32 count; // 引用计数
  time_t atime; // 访问时间
  time_t ctime; // 修改时间
  list_node_t node;
  dev_t mount; // 挂载的设备
} inode_t;

typedef struct super_desc_t {
  uint16 inodes;        // inode节点数
  uint16 zones;         // 逻辑块数
  uint16 imap_blocks;   // i节点位图所占用的数据块数
  uint16 zmap_blocks;   // 逻辑块位图所占用的数据块数
  uint16 firstdatazone; // 第一个数据逻辑块号
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
  list_t inode_list; // 打开的inode链表
  inode_t *iroot;    // 根目录inode
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
  SEEK_SET = 1, // 直接设置偏移
  SEEK_CUR,     // 当前位置偏移
  SEEK_END      // 结束位置偏移
} whence_t;

super_block_t *get_super(dev_t dev);
super_block_t *read_super(dev_t dev);

// 操作位图
idx_t balloc(dev_t dev);
void bfree(dev_t dev, idx_t idx);
idx_t ialloc(dev_t dev);
void ifree(dev_t dev, idx_t idx);
idx_t bmap(inode_t *inode, idx_t block, bool create);

inode_t *get_root_inode(); // 根目录inode
inode_t *iget(dev_t dev, idx_t nr);
void iput(inode_t *inode);

inode_t *named(char *pathname, char **next);
inode_t *namei(char *pathname);

inode_t *inode_open(char *pathname, int flag, int mode);
int inode_read(inode_t *inode, char *buf, uint32 len, off_t offset);
int inode_write(inode_t *inode, char *buf, uint32 len, off_t offset);
void inode_truncate(inode_t *inode);

#endif