#include "../include/conix/ide.h"
#include "../include/conix/assert.h"
#include "../include/conix/debug.h"
#include "../include/conix/device.h"
#include "../include/conix/interrupt.h"
#include "../include/conix/io.h"
#include "../include/conix/memory.h"
#include "../include/conix/stdio.h"
#include "../include/conix/string.h"

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_CHS_SECTOR 0x0003 // CHS 扇区位置
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_CHS_CYL 0x0004    // CHS 柱面低字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_CHS_CYH 0x0005    // CHS 柱面高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令
#define IDE_CMD_READ 0x20       // 读命令
#define IDE_CMD_WRITE 0x30      // 写命令
#define IDE_CMD_IDENTIFY 0xEC   // 识别命令
#define IDE_CMD_DIAGNOSTIC 0x90 // 诊断命令

#define IDE_CMD_READ_UDMA 0xC8  // UDMA 读命令
#define IDE_CMD_WRITE_UDMA 0xCA // UDMA 写命令

#define IDE_CMD_PIDENTIFY 0xA1 // 识别 PACKET 命令
#define IDE_CMD_PACKET 0xA0    // PACKET 命令

// ATAPI 命令
#define IDE_ATAPI_CMD_REQUESTSENSE 0x03
#define IDE_ATAPI_CMD_READCAPICITY 0x25
#define IDE_ATAPI_CMD_READ10 0x28

#define IDE_ATAPI_FEATURE_PIO 0
#define IDE_ATAPI_FEATURE_DMA 1

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA
#define IDE_SEL_MASK 0b10110000   // CHS 模式 MASK

#define IDE_INTERFACE_UNKNOWN 0
#define IDE_INTERFACE_ATA 1
#define IDE_INTERFACE_ATAPI 2

// 总线主控寄存器偏移
#define BM_COMMAND_REG 0 // 命令寄存器偏移
#define BM_STATUS_REG 2  // 状态寄存器偏移
#define BM_PRD_ADDR 4    // PRD 地址寄存器偏移

// 总线主控命令寄存器
#define BM_CR_STOP 0x00  // 终止传输
#define BM_CR_START 0x01 // 开始传输
#define BM_CR_WRITE 0x00 // 主控写磁盘
#define BM_CR_READ 0x08  // 主控读磁盘

// 总线主控状态寄存器
#define BM_SR_ACT 0x01     // 激活
#define BM_SR_ERR 0x02     // 错误
#define BM_SR_INT 0x04     // 中断信号生成
#define BM_SR_DRV0 0x20    // 驱动器 0 可以使用 DMA 方式
#define BM_SR_DRV1 0x40    // 驱动器 1 可以使用 DMA 方式
#define BM_SR_SIMPLEX 0x80 // 仅单纯形操作

// 分区文件系统
typedef enum PART_FS {
  PART_FS_FAT12 = 1,    // FAT12
  PART_FS_EXTENDED = 5, // 扩展分区
  PART_FS_MINIX = 0x80, // minux
  PART_FS_LINUX = 0x83, // linux
} PART_FS;

ide_ctrl_t controllers[IDE_CTRL_NR];

static void ide_part_init(ide_disk_t *disk, uint16 *buf) {
  // 磁盘不可用
  if (!disk->total_lba)
    return;

  // 读取主引导扇区
  ide_pio_read(disk, buf, 1, 0);

  // 初始化主引导扇区
  boot_sector_t *boot = (boot_sector_t *)buf;

  for (size_t i = 0; i < IDE_PART_NR; i++) {
    part_entry_t *entry = &boot->entry[i];
    ide_part_t *part = &disk->parts[i];
    if (!entry->count)
      continue;

    sprintf(part->name, "%s%d", disk->name, i + 1);

    LOGK("part %s \n", part->name);
    LOGK("    bootable %d\n", entry->bootable);
    LOGK("    start %d\n", entry->start);
    LOGK("    count %d\n", entry->count);
    LOGK("    system 0x%x\n", entry->system);

    part->disk = disk;
    part->count = entry->count;
    part->system = entry->system;
    part->start = entry->start;

    if (entry->system == PART_FS_EXTENDED) {
      LOGK("Unsupported extended partition!!!\n");

      boot_sector_t *eboot = (boot_sector_t *)(buf + SECTOR_SIZE);
      ide_pio_read(disk, (void *)eboot, 1, entry->start);

      for (size_t j = 0; j < IDE_PART_NR; j++) {
        part_entry_t *eentry = &eboot->entry[j];
        if (!eentry->count)
          continue;
        LOGK("part %d extend %d\n", i, j);
        LOGK("    bootable %d\n", eentry->bootable);
        LOGK("    start %d\n", eentry->start);
        LOGK("    count %d\n", eentry->count);
        LOGK("    system 0x%x\n", eentry->system);
      }
    }
  }
}

static void ide_ctrl_init() {
  uint16 *buf = (uint16 *)alloc_kpage(1);
  for (size_t cidx = 0; cidx < IDE_CTRL_NR; ++cidx) {
    ide_ctrl_t *ctrl = &controllers[cidx];
    sprintf(ctrl->name, "ide%u", cidx);
    lock_init(&ctrl->lock);
    ctrl->active = NULL;

    if (cidx) {
      ctrl->iobase = IDE_IOBASE_SECONDARY;
    } else {
      ctrl->iobase = IDE_IOBASE_PRIMARY;
    }

    for (size_t didx = 0; didx < IDE_DISK_NR; ++didx) {
      ide_disk_t *disk = &ctrl->disks[didx];
      sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);
      disk->ctrl = ctrl;
      if (didx) {
        disk->master = false;
        disk->selector = IDE_LBA_SLAVE;
      } else {
        disk->master = true;
        disk->selector = IDE_LBA_MASTER;
      }
      ide_part_init(disk, buf);
    }
  }

  free_kpage((uint32)buf, 1);
}

static void ide_select_drive(ide_disk_t *disk) {
  outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
  disk->ctrl->active = disk;
}

static uint32 ide_error(ide_ctrl_t *ctrl) {
  uint8 error = inb(ctrl->iobase + IDE_ERR);
  if (error & IDE_ER_BBK)
    LOGK("bad block\n");
  if (error & IDE_ER_UNC)
    LOGK("uncorrectable data\n");
  if (error & IDE_ER_MC)
    LOGK("media change\n");
  if (error & IDE_ER_IDNF)
    LOGK("id not found\n");
  if (error & IDE_ER_MCR)
    LOGK("media change requested\n");
  if (error & IDE_ER_ABRT)
    LOGK("abort\n");
  if (error & IDE_ER_TK0NF)
    LOGK("track 0 not found\n");
  if (error & IDE_ER_AMNF)
    LOGK("address mark not found\n");
}

static uint32 ide_busy_wait(ide_ctrl_t *ctrl, uint8 mask) {
  while (1) {
    uint8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
    if (state & IDE_SR_ERR) {
      ide_error(ctrl);
    }
    if (state & IDE_SR_BSY) {
      continue;
    }
    if ((state & mask) == mask) {
      return 0;
    }
  }
}

static void ide_select_sector(ide_disk_t *disk, uint32 lba, uint8 count) {
  // 输出功能，可省略
  outb(disk->ctrl->iobase + IDE_FEATURE, 0);

  // 读写扇区数量
  outb(disk->ctrl->iobase + IDE_SECTOR, count);

  // LBA 低字节
  outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
  // LBA 中字节
  outb(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
  // LBA 高字节
  outb(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

  // LBA 最高四位 + 磁盘选择
  outb(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);
  disk->ctrl->active = disk;
}

static void ide_pio_read_selector(ide_disk_t *disk, uint16 *buf) {
  for (size_t i = 0; i < (SECTOR_SIZE / 2); ++i) {
    buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
  }
}

int ide_pio_read(ide_disk_t *disk, void *buf, uint8 count, idx_t lba) {
  assert(count > 0);
  assert(!get_interrupt_state());

  ide_ctrl_t *ctrl = disk->ctrl;
  lock_acquire(&ctrl->lock);

  ide_select_drive(disk);
  ide_busy_wait(ctrl, IDE_SR_DRDY);
  ide_select_sector(disk, lba, count);

  // 发送读命令
  outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

  for (size_t i = 0; i < count; ++i) {
    task_t *task = running_task();
    if (task->state == TASK_RUNNING) {
      ctrl->waiter = task;
      task_block(task, NULL, TASK_BLOCKED);
    }

    ide_busy_wait(ctrl, IDE_SR_DRQ);
    uint32 offset = ((uint32)buf + i * SECTOR_SIZE);
    ide_pio_read_selector(disk, (uint16 *)offset);
  }

  lock_release(&ctrl->lock);
  return 0;
}

static void ide_pio_write_sector(ide_disk_t *disk, uint16 *buf) {
  for (size_t i = 0; i < (SECTOR_SIZE / 2); i++) {
    outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
  }
}

int ide_pio_ioctl(ide_disk_t *disk, int cmd, void *args, int flags) {
  switch (cmd) {
  case DEV_CMD_SECTOR_START:
    return 0;
  case DEV_CMD_SECTOR_COUNT:
    return disk->total_lba;
  default:
    panic("no command");
  }
}

int ide_pio_write(ide_disk_t *disk, void *buf, uint8 count, idx_t lba) {
  assert(count > 0);
  assert(!get_interrupt_state());

  ide_ctrl_t *ctrl = disk->ctrl;
  lock_acquire(&ctrl->lock);

  ide_select_drive(disk);
  ide_busy_wait(ctrl, IDE_SR_DRDY);
  ide_select_sector(disk, lba, count);

  // 发送写命令
  outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

  for (size_t i = 0; i < count; ++i) {
    uint32 offset = ((uint32)buf + i * SECTOR_SIZE);
    ide_pio_write_sector(disk, (uint16 *)offset);

    task_t *task = running_task();
    if (task->state == TASK_RUNNING) {
      ctrl->waiter = task;
      task_block(task, NULL, TASK_BLOCKED);
    }

    ide_busy_wait(ctrl, IDE_SR_NULL);
  }
  lock_release(&ctrl->lock);
  return 0;
}

void ide_handler(int vec) {
  send_eoi(vec);

  ide_ctrl_t *ctrl = &controllers[vec - IRQ_HARDDISK - 0x20];

  uint8 state = inb(ctrl->iobase + IDE_STATUS);
  if (ctrl->waiter) {
    task_unblock(ctrl->waiter);
    ctrl->waiter = NULL;
  }
}

int ide_pio_part_ioctl(ide_part_t *part, int cmd, void *args, int flags) {
  switch (cmd) {
  case DEV_CMD_SECTOR_START:
    return part->start;
  case DEV_CMD_SECTOR_COUNT:
    return part->count;
  default:
    panic("no command");
  }
}

// 读分区
int ide_pio_part_read(ide_part_t *part, void *buf, uint8 count, idx_t lba) {
  return ide_pio_read(part->disk, buf, count, part->start + lba);
}

// 写分区
int ide_pio_part_write(ide_part_t *part, void *buf, uint8 count, idx_t lba) {
  return ide_pio_write(part->disk, buf, count, part->start + lba);
}

static void ide_intall() {
  for (size_t cidx = 0; cidx < IDE_CTRL_NR; ++cidx) {
    ide_ctrl_t *ctrl = &controllers[cidx];
    for (size_t didx = 0; didx < IDE_DISK_NR; ++didx) {
      ide_disk_t *disk = &ctrl->disks[didx];
      if (!disk->total_lba) {
        continue;
      }
      dev_t dev = device_install(DEV_BLOCK, DEV_IDE_DISK, disk, disk->name, 0,
                                 ide_pio_ioctl, ide_pio_read, ide_pio_write);

      for (size_t i = 0; i < IDE_PART_NR; ++i) {
        ide_part_t *part = &disk->parts[i];
        if (!part->count) {
          continue;
        }

        device_install(DEV_BLOCK, DEV_IDE_PART, part, part->name, dev,
                       ide_pio_part_ioctl, ide_pio_part_read,
                       ide_pio_part_write);
      }
    }
  }
}

void ide_init() {
  ide_ctrl_init();

  set_interrupt_handler(IRQ_HARDDISK, ide_handler);
  set_interrupt_handler(IRQ_HARDDISK2, ide_handler);

  ide_intall();

  set_interrupt_mask(IRQ_HARDDISK, true);
  set_interrupt_mask(IRQ_HARDDISK2, true);
  set_interrupt_mask(IRQ_CASCADE, true);
}