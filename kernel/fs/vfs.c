#include <kernel/fs/vfs.h>
#include <kernel/mm/kheap.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── VFS globals ────────────────────────────────────────────────────────── */

vfs_node_t *vfs_root = NULL;

static filesystem_t *fs_registry[16];
static int fs_count = 0;

/* Mount table */
typedef struct {
    char        path[VFS_PATH_MAX];
    vfs_node_t *root;
} mount_entry_t;

static mount_entry_t mount_table[32];
static int mount_count = 0;

/* ─── Registration ───────────────────────────────────────────────────────── */

void vfs_register_fs(filesystem_t *fs) {
    if (fs_count < 16) {
        fs_registry[fs_count++] = fs;
        pr_info("VFS: registered filesystem '%s'\n", fs->name);
    }
}

/* ─── Mount / unmount ────────────────────────────────────────────────────── */

int vfs_mount(const char *path, vfs_node_t *root) {
    if (!path || !root) return -EINVAL;
    if (mount_count >= 32) return -ENOSPC;

    if (strcmp(path, "/") == 0) {
        vfs_root = root;
    }

    strncpy(mount_table[mount_count].path, path, VFS_PATH_MAX - 1);
    mount_table[mount_count].root = root;
    mount_count++;
    pr_info("VFS: mounted '%s' on %s\n", root->name, path);
    return 0;
}

int vfs_unmount(const char *path) {
    for (int i = 0; i < mount_count; i++) {
        if (strcmp(mount_table[i].path, path) == 0) {
            for (int j = i; j < mount_count - 1; j++)
                mount_table[j] = mount_table[j + 1];
            mount_count--;
            return 0;
        }
    }
    return -ENOENT;
}

/* ─── Path resolution ────────────────────────────────────────────────────── */

static vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path || !vfs_root) return NULL;

    if (path[0] == '/' && path[1] == '\0') return vfs_root;

    vfs_node_t *node = vfs_root;
    char buf[VFS_PATH_MAX];
    strncpy(buf, path, VFS_PATH_MAX - 1);

    /* Skip leading '/' */
    char *tok = buf + (buf[0] == '/' ? 1 : 0);
    while (tok && *tok) {
        char *slash = strchr(tok, '/');
        if (slash) *slash = '\0';

        if (!node->ops || !node->ops->finddir) return NULL;
        node = node->ops->finddir(node, tok);
        if (!node) return NULL;

        tok = slash ? slash + 1 : NULL;
    }
    return node;
}

/* ─── Public API ─────────────────────────────────────────────────────────── */

void vfs_init(void) {
    memset(fs_registry,  0, sizeof(fs_registry));
    memset(mount_table,  0, sizeof(mount_table));
    pr_info("VFS initialised\n");
}

vfs_node_t *vfs_open(const char *path, uint32_t flags) {
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) return NULL;
    if (node->ops && node->ops->open)
        node->ops->open(node, flags);
    node->ref_count++;
    return node;
}

int vfs_close(vfs_node_t *node) {
    if (!node) return -EINVAL;
    if (node->ref_count > 0) node->ref_count--;
    if (node->ops && node->ops->close)
        return node->ops->close(node);
    return 0;
}

ssize_t vfs_read(vfs_node_t *node, off_t offset, size_t size, void *buf) {
    if (!node || !node->ops || !node->ops->read) return -ENOSYS;
    return node->ops->read(node, offset, size, buf);
}

ssize_t vfs_write(vfs_node_t *node, off_t offset, size_t size, const void *buf) {
    if (!node || !node->ops || !node->ops->write) return -ENOSYS;
    return node->ops->write(node, offset, size, buf);
}

int vfs_stat(const char *path, vfs_stat_t *st) {
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) return -ENOENT;
    if (!node->ops || !node->ops->stat) {
        /* Fill from node directly */
        st->size  = node->size;
        st->inode = node->inode;
        st->mode  = node->mode;
        return 0;
    }
    return node->ops->stat(node, st);
}

int vfs_mkdir(const char *path, uint32_t mode) {
    char dir_buf[VFS_PATH_MAX], base_buf[VFS_PATH_MAX];
    vfs_path_dirname(path, dir_buf, VFS_PATH_MAX);
    vfs_path_basename(path, base_buf, VFS_PATH_MAX);

    vfs_node_t *parent = vfs_resolve_path(dir_buf);
    if (!parent) return -ENOENT;
    if (!parent->ops || !parent->ops->mkdir) return -ENOSYS;
    return parent->ops->mkdir(parent, base_buf, mode);
}

int vfs_unlink(const char *path) {
    char dir_buf[VFS_PATH_MAX], base_buf[VFS_PATH_MAX];
    vfs_path_dirname(path, dir_buf, VFS_PATH_MAX);
    vfs_path_basename(path, base_buf, VFS_PATH_MAX);

    vfs_node_t *parent = vfs_resolve_path(dir_buf);
    if (!parent) return -ENOENT;
    if (!parent->ops || !parent->ops->unlink) return -ENOSYS;
    return parent->ops->unlink(parent, base_buf);
}

int vfs_rename(const char *old_path, const char *new_path) {
    (void)old_path; (void)new_path;
    return -ENOSYS;   /* TODO */
}

vfs_node_t *vfs_readdir(vfs_node_t *dir, uint32_t index, vfs_dirent_t *dirent) {
    if (!dir || !dir->ops || !dir->ops->readdir) return NULL;
    dir->ops->readdir(dir, index, dirent);
    return dir->ops->finddir ? dir->ops->finddir(dir, dirent->name) : NULL;
}

vfs_node_t *vfs_finddir(vfs_node_t *dir, const char *name) {
    if (!dir || !dir->ops || !dir->ops->finddir) return NULL;
    return dir->ops->finddir(dir, name);
}

/* ─── Path utilities ─────────────────────────────────────────────────────── */

void vfs_path_dirname(const char *path, char *buf, size_t bufsz) {
    strncpy(buf, path, bufsz - 1);
    char *slash = strrchr(buf, '/');
    if (!slash) { buf[0] = '.'; buf[1] = '\0'; return; }
    if (slash == buf) { buf[1] = '\0'; return; }
    *slash = '\0';
}

void vfs_path_basename(const char *path, char *buf, size_t bufsz) {
    const char *slash = strrchr(path, '/');
    strncpy(buf, slash ? slash + 1 : path, bufsz - 1);
    buf[bufsz - 1] = '\0';
}

int vfs_path_canonicalize(const char *path, char *buf, size_t bufsz) {
    strncpy(buf, path, bufsz - 1);
    buf[bufsz - 1] = '\0';
    return 0;  /* TODO: resolve .. and . components */
}
