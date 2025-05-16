#ifndef LEDANIM_H
#define LEDANIM_H

#include <linux/types.h>
#include <linux/spi/spi.h>
#include <linux/workqueue.h>

enum anim_mode {
    ANIM_NONE,
    ANIM_BLINK,
    ANIM_FADE,
    ANIM_PULSE,
};

int  ledanim_init(struct spi_device *spi);
void ledanim_stop(void);
int  ledanim_start(enum anim_mode mode,
                   unsigned long period_ms,
                   const u8 *frame,
                   size_t len);

/* New APIs for live color updates */
bool ledanim_is_active(void);
int  ledanim_update_frame(const u8 *frame, size_t len);

#endif /* LEDANIM_H */
