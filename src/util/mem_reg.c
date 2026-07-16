#include "mem_reg.h"

#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

// Map the 4KB page containing addr and run op on the register. Each call
// opens /dev/mem, maps one page and unmaps it again: that is tens of
// microseconds, versus the ~15-25ms fork+exec+shell of spawning `aww`/`awr`
// per poke, and keeping no persistent mapping means no global state to guard.
static int mem_reg_op(uint32_t addr, uint32_t *val, int is_write) {
    const uint32_t page_size = (uint32_t)sysconf(_SC_PAGESIZE);
    const uint32_t page_base = addr & ~(page_size - 1);
    const uint32_t page_off = addr - page_base;

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
        return -1;

    volatile uint8_t *map = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, page_base);
    if (map == MAP_FAILED) {
        close(fd);
        return -1;
    }

    volatile uint32_t *reg = (volatile uint32_t *)(map + page_off);
    if (is_write)
        *reg = *val;
    else
        *val = *reg;

    munmap((void *)map, page_size);
    close(fd);
    return 0;
}

int hw_reg_write(uint32_t addr, uint32_t val) {
    return mem_reg_op(addr, &val, 1);
}

int hw_reg_read(uint32_t addr, uint32_t *val) {
    return mem_reg_op(addr, val, 0);
}
