#include "gpadc.h"

#if defined(HDZBOXPRO) || defined(HDZGOGGLE2)

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <log/log.h>

#include "../core/common.hh"
#include "util/mem_reg.h"
#include "util/system.h"

void gpadc_init() {
    // open gpadc clock
    hw_reg_write(0x030019ec, 0x00010001);

    // open adc channel 0
    hw_reg_write(0x05070008, 0x00000001);
}

void gpadc_on(uint8_t is_on) {
    hw_reg_write(0x05070004, is_on ? 0xffbd0000 : 0xffbc0000);
}

int gpdac0_get() {
#ifdef EMULATOR_BUILD
    return -1;
#endif

    // was: spawn `awr` redirected to a file, sleep 10ms, fopen+fscanf it back
    uint32_t reg;
    if (hw_reg_read(0x05070080, &reg) != 0)
        return -1;

    return reg;
}
#else
void gpadc_init() {}
void gpadc_on(uint8_t is_on) {}
int gpdac0_get() { return -1; }
#endif
