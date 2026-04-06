#include <kernel/driver.h>
#include <kernel/mm/kheap.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── Driver registry ────────────────────────────────────────────────────── */

static driver_t *driver_list_head = NULL;

void driver_register(driver_t *drv) {
    if (!drv) return;
    drv->next   = driver_list_head;
    driver_list_head = drv;
    pr_info("Driver registered: '%s' (type %d)\n", drv->name, drv->type);
}

void driver_unregister(driver_t *drv) {
    driver_t **pp = &driver_list_head;
    while (*pp) {
        if (*pp == drv) { *pp = drv->next; return; }
        pp = &(*pp)->next;
    }
}

driver_t *driver_find(const char *name) {
    driver_t *d = driver_list_head;
    while (d) {
        if (strcmp(d->name, name) == 0) return d;
        d = d->next;
    }
    return NULL;
}

driver_t *driver_find_by_type(driver_type_t type, uint32_t index) {
    uint32_t count = 0;
    driver_t *d = driver_list_head;
    while (d) {
        if (d->type == type) {
            if (count == index) return d;
            count++;
        }
        d = d->next;
    }
    return NULL;
}

void drivers_probe_all(void) {
    driver_t *d = driver_list_head;
    int ok = 0, fail = 0;
    while (d) {
        if (d->probe) {
            int ret = d->probe(d);
            if (ret == 0) {
                d->state = DRIVER_RUNNING;
                ok++;
            } else {
                d->state = DRIVER_ERROR;
                pr_warn("Driver probe failed: '%s' (err %d)\n", d->name, ret);
                fail++;
            }
        } else {
            d->state = DRIVER_RUNNING;   /* No probe needed */
            ok++;
        }
        d = d->next;
    }
    pr_info("Drivers: %d ready, %d failed\n", ok, fail);
}

void drivers_list(void) {
    driver_t *d = driver_list_head;
    printk("NAME                  TYPE   STATE\n");
    printk("--------------------  -----  -------\n");
    while (d) {
        const char *types[]  = {"CHAR","BLOCK","NET","DISPLAY","INPUT","BUS","MISC"};
        const char *states[] = {"UNLOADED","LOADED","RUNNING","ERROR"};
        printk("%-20s  %-5s  %s\n",
               d->name,
               types[d->type < 7 ? d->type : 6],
               states[d->state < 4 ? d->state : 0]);
        d = d->next;
    }
}
