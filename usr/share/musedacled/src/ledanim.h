#ifndef LEDANIM_H
#define LEDANIM_H

#include <linux/types.h>

enum anim_mode {
    ANIM_NONE,
    ANIM_BLINK,
    ANIM_FADE,
    ANIM_PULSE,
};

bool ledanim_is_active(void);
int  ledanim_update_frame(const u8 *frame, size_t len);
int ledanim_init(struct spi_device *spi);
void ledanim_stop(void);
int ledanim_start(enum anim_mode mode, unsigned long delay_ms, const u8 *frame, size_t len);
extern enum anim_mode current_mode;

#endif
