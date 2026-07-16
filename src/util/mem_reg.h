#ifndef _MEM_REG_H
#define _MEM_REG_H

#include <stdint.h>

// Direct 32-bit physical register access via /dev/mem — functional twin of
// the `aww`/`awr` helper binaries without the fork+exec+shell round trip.
// Both return 0 on success, -1 on failure (e.g. no /dev/mem in the emulator).
int hw_reg_write(uint32_t addr, uint32_t val);
int hw_reg_read(uint32_t addr, uint32_t *val);

#endif
