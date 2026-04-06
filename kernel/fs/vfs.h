#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <kernel/types.h>

/* VFS node types */
#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEV     0x03
#define VFS_BLOCKDEV    0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_MOUNTPOINT  0x08

/* Open flags */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_TRUNC     0x0200
#define O_APPEND    0x0400
#define O_NONBLOCK  0x0800
#define O_DIRECTORY 0x10000

/* Seek whence */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define VFS_NAME_MAX  255
#define VFS_PATH_MAX  4096

/* Forward declarations */
typedef struct vfs_node vfs_node_t;
typedef struct vfs_ops  vfs_ops_t;

/* File stat */
typedef struct {
    uint32_t mode;
    uint32_t uid, gid;
    uint64_t size;
    uint64_t inode;
    uint64_t atime, mtime, ctime;
    uint32_t nlinks;
} vfs_stat_t;

/* Directory entry */
typedef struct {
    uint64_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  type;
    char     name[VFS_NAME_MAX + 1];
} vfs_dirent_t;

/* VFS operations table (function pointers per filesystem) */
struct vfs_ops {
    ssize_t   (*read)(vfs_node_t *node, off_t offset, size_t size, void *buf);
    ssize_t   (*write)(vfs_node_t *node, off_t offset, size_t size, const void *buf);
    int       (*open)(vfs_node_t *node, uint32_t flags);
    int       (*close)(vfs_node_t *node);
    int       (*stat)(vfs_node_t *node, vfs_stat_t *st);
    int       (*readdir)(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent);
    vfs_node_t *(*finddir)(vfs_node_t *node, const char *name);
    int       (*mkdir)(vfs_node_t *parent, const char *name, uint32_t mode);
    int       (*unlink)(vfs_node_t *parent, const char *name);
    int       (*rename)(vfs_node_t *old_dir, const char *old_name,
                        vfs_node_t *new_dir, const char *new_name);
    int       (*truncate)(vfs_node_t *node, uint64_t size);
    int       (*ioctl)(vfs_node_t *node, uint32_t cmd, void *arg);
};

/* VFS Node (inode abstraction) */
struct vfs_node {
    char        name[VFS_NAME_MAX + 1];
    uint32_t    type;
    uint32_t    flags;
    uint64_t    inode;
    uint64_t    size;
    uint32_t    uid, gid;
    uint32_t    mode;
    uint64_t    atime, mtime, ctime;
    vfs_ops_t  *ops;
    vfs_node_t *mountpoint;  /* If this is a mountpoint, points to mounted root */
    void       *fs_data;     /* Filesystem-private data */
    uint32_t    ref_count;
};

/* Filesystem registration */
typedef struct {
    const char  *name;              /* e.g. "ext2", "tmpfs", "fat32" */
    vfs_node_t *(*mount)(const char *device, uint32_t flags);
    int          (*unmount)(vfs_node_t *root);
} filesystem_t;

/* VFS public API */
void        vfs_init(void);
int         vfs_mount(const char *path, vfs_node_t *root);
int         vfs_unmount(const char *path);
void        vfs_register_fs(filesystem_t *fs);

vfs_node_t *vfs_open(const char *path, uint32_t flags);
int         vfs_close(vfs_node_t *node);
ssize_t     vfs_read(vfs_node_t *node, off_t offset, size_t size, void *buf);
ssize_t     vfs_write(vfs_node_t *node, off_t offset, size_t size, const void *buf);
int         vfs_stat(const char *path, vfs_stat_t *st);
int         vfs_mkdir(const char *path, uint32_t mode);
int         vfs_unlink(const char *path);
int         vfs_rename(const char *old_path, const char *new_path);
vfs_node_t *vfs_readdir(vfs_node_t *dir, uint32_t index, vfs_dirent_t *dirent);
vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name);

/* Path utilities */
void  vfs_path_dirname(const char *path, char *buf, size_t bufsz);
void  vfs_path_basename(const char *path, char *buf, size_t bufsz);
int   vfs_path_canonicalize(const char *path, char *buf, size_t bufsz);

extern vfs_node_t *vfs_root;   /* "/" */

#endif /* KERNEL_VFS_H */
