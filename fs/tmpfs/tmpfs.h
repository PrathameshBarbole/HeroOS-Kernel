#ifndef TMPFS_H
#define TMPFS_H

#include <kernel/fs/vfs.h>

void        tmpfs_register(void);
vfs_node_t *tmpfs_create_root(void);

#endif /* TMPFS_H */
