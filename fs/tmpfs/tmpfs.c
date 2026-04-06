/*
 * tmpfs — in-memory filesystem backed by the kernel heap.
 * Used for /tmp and /dev initially.
 */

#include <fs/tmpfs/tmpfs.h>
#include <kernel/mm/kheap.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── tmpfs inode ────────────────────────────────────────────────────────── */

#define TMPFS_MAX_CHILDREN  64
#define TMPFS_INITIAL_BUF   256

typedef struct tmpfs_inode {
    vfs_node_t   vnode;                          /* Must be first member */
    uint8_t     *data;                           /* File data buffer */
    size_t       capacity;
    struct tmpfs_inode *children[TMPFS_MAX_CHILDREN];
    uint32_t     child_count;
    uint64_t     inode_no;
} tmpfs_inode_t;

static uint64_t tmpfs_inode_counter = 1;

/* ─── VFS operations ─────────────────────────────────────────────────────── */

static ssize_t tmpfs_read(vfs_node_t *node, off_t offset, size_t size, void *buf) {
    tmpfs_inode_t *inode = (tmpfs_inode_t *)node;
    if (!inode->data || (uint64_t)offset >= node->size) return 0;
    size_t avail = (size_t)(node->size - (uint64_t)offset);
    size_t to_read = MIN(size, avail);
    memcpy(buf, inode->data + offset, to_read);
    return (ssize_t)to_read;
}

static ssize_t tmpfs_write(vfs_node_t *node, off_t offset, size_t size, const void *buf) {
    tmpfs_inode_t *inode = (tmpfs_inode_t *)node;
    uint64_t new_end = (uint64_t)offset + size;

    if (new_end > inode->capacity) {
        size_t new_cap = (size_t)ALIGN_UP(new_end, 512);
        uint8_t *new_data = (uint8_t *)krealloc(inode->data, new_cap);
        if (!new_data) return -12;  /* ENOMEM */
        inode->data     = new_data;
        inode->capacity = new_cap;
    }
    memcpy(inode->data + offset, buf, size);
    if (new_end > node->size) node->size = new_end;
    return (ssize_t)size;
}

static int tmpfs_open(vfs_node_t *node, uint32_t flags) {
    (void)node; (void)flags; return 0;
}

static int tmpfs_close(vfs_node_t *node) {
    (void)node; return 0;
}

static int tmpfs_stat(vfs_node_t *node, vfs_stat_t *st) {
    st->inode  = node->inode;
    st->size   = node->size;
    st->mode   = node->mode;
    st->nlinks = 1;
    return 0;
}

static int tmpfs_readdir(vfs_node_t *node, uint32_t index, vfs_dirent_t *dirent) {
    tmpfs_inode_t *inode = (tmpfs_inode_t *)node;
    if (index >= inode->child_count) return -1;
    tmpfs_inode_t *child = inode->children[index];
    strncpy(dirent->name, child->vnode.name, VFS_NAME_MAX);
    dirent->inode = child->vnode.inode;
    dirent->type  = (uint8_t)child->vnode.type;
    return 0;
}

static vfs_node_t *tmpfs_finddir(vfs_node_t *node, const char *name) {
    tmpfs_inode_t *inode = (tmpfs_inode_t *)node;
    for (uint32_t i = 0; i < inode->child_count; i++) {
        if (strcmp(inode->children[i]->vnode.name, name) == 0)
            return &inode->children[i]->vnode;
    }
    return NULL;
}

static int tmpfs_mkdir(vfs_node_t *parent, const char *name, uint32_t mode) {
    tmpfs_inode_t *p = (tmpfs_inode_t *)parent;
    if (p->child_count >= TMPFS_MAX_CHILDREN) return -28;  /* ENOSPC */

    tmpfs_inode_t *child = (tmpfs_inode_t *)kcalloc(1, sizeof(tmpfs_inode_t));
    if (!child) return -12;

    strncpy(child->vnode.name, name, VFS_NAME_MAX);
    child->vnode.type   = VFS_DIRECTORY;
    child->vnode.mode   = mode;
    child->vnode.inode  = tmpfs_inode_counter++;
    child->vnode.ops    = parent->ops;
    child->inode_no     = child->vnode.inode;

    p->children[p->child_count++] = child;
    return 0;
}

static int tmpfs_unlink(vfs_node_t *parent, const char *name) {
    tmpfs_inode_t *p = (tmpfs_inode_t *)parent;
    for (uint32_t i = 0; i < p->child_count; i++) {
        if (strcmp(p->children[i]->vnode.name, name) == 0) {
            tmpfs_inode_t *victim = p->children[i];
            kfree(victim->data);
            kfree(victim);
            /* Shift remaining entries */
            for (uint32_t j = i; j < p->child_count - 1; j++)
                p->children[j] = p->children[j + 1];
            p->child_count--;
            return 0;
        }
    }
    return -2;  /* ENOENT */
}

static vfs_ops_t tmpfs_ops = {
    .read    = tmpfs_read,
    .write   = tmpfs_write,
    .open    = tmpfs_open,
    .close   = tmpfs_close,
    .stat    = tmpfs_stat,
    .readdir = tmpfs_readdir,
    .finddir = tmpfs_finddir,
    .mkdir   = tmpfs_mkdir,
    .unlink  = tmpfs_unlink,
};

/* ─── Filesystem mount ───────────────────────────────────────────────────── */

static vfs_node_t *tmpfs_mount(const char *device, uint32_t flags) {
    (void)device; (void)flags;

    tmpfs_inode_t *root = (tmpfs_inode_t *)kcalloc(1, sizeof(tmpfs_inode_t));
    if (!root) return NULL;

    strncpy(root->vnode.name, "tmpfs", VFS_NAME_MAX);
    root->vnode.type  = VFS_DIRECTORY;
    root->vnode.mode  = 0755;
    root->vnode.inode = tmpfs_inode_counter++;
    root->vnode.ops   = &tmpfs_ops;
    root->inode_no    = root->vnode.inode;

    pr_info("tmpfs: mounted in-memory filesystem\n");
    return &root->vnode;
}

static int tmpfs_unmount(vfs_node_t *root) {
    kfree(root);
    return 0;
}

static filesystem_t tmpfs_fs = {
    .name    = "tmpfs",
    .mount   = tmpfs_mount,
    .unmount = tmpfs_unmount,
};

void tmpfs_register(void) {
    vfs_register_fs(&tmpfs_fs);
}

vfs_node_t *tmpfs_create_root(void) {
    return tmpfs_mount(NULL, 0);
}
