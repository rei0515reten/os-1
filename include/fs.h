#ifndef FS_H
#define FS_H

#include <types.h>

struct super_block {
	u32 ninodes;
	u16 nzones;
	u16 imap_blocks;
	u16 zmap_blocks;
	u16 firstdatazone;
	u16 log_zone_size;
	u16 pad;
	u32 max_size;
	u32 zones;
	u16 magic;
	u16 pad2;
	u16 block_size;
	u8 disk_version;
};

#define NINODE 16
struct inode {
	u16 mode;
	u16 nlinks;
	u16 uid;
	u16 gid;
	u32 size;
	u32 atime;
	u32 mtime;
	u32 ctime;
	u32 zone[10];
};

void read_super(void);
void fsinit(void);
struct inode *iget(u32 dev, u64 inum);
struct inode *namei(char *path);
struct inode *diri(struct inode *ip, char *name);
u8 bmapget(u64 bmap, u64 inum);
u64 zmap(struct inode *ip, u64 zone);
char *dirname(char *path);
char *basename(char *path);

u64 readi(struct inode *ip, char *dest, u64 offset, u64 size);

extern struct inode inode[NINODE];

/* 
readi();
writei();
openi();
closei();
bmap();
namei();
*/

#define IMAP(sb) (2)
#define ZMAP(sb) (2+sb->imap_blocks)

#define S_IFDIR 0x6000

#define DIRSIZ 60

#define SUPERBLOCK 2
#define ROOT 1

struct direct {
	u32 ino;
	char name[DIRSIZ];
};

#endif
