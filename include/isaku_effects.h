#ifndef ISAKU_EFFECTS_H
#define ISAKU_EFFECTS_H
#include <stdint.h>
/* AI5WIN 004099f0..00409aa0: each BGR555 channel is weighted /31. */
static inline uint8_t isaku_blend_channel(uint8_t a, uint8_t b, unsigned weight)
{
    return (uint8_t)((((a >> 3) * weight + (b >> 3) * (31 - weight)) / 31) << 3);
}
#endif
