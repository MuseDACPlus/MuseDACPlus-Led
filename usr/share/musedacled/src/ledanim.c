#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/jiffies.h>
#include <linux/string.h>
#include "ledanim.h"

static struct spi_device *anim_spi      = NULL;
static u8               *base_frame    = NULL;
static size_t            frame_len     = 0;
static unsigned long     delay_ms      = 0;
static unsigned long     period_ms     = 0;  /* Store original period for read */
static unsigned int     anim_t         = 0;
static bool             anim_state     = false;
static enum anim_mode   current_mode   = ANIM_NONE;
static struct delayed_work anim_work;

/* Animation worker: one step, then reschedule */
static void anim_worker(struct work_struct *work)
{
    u8 *frame;
    size_t i;

    if (!base_frame || !anim_spi || frame_len < 8)
        return;

    /* Copy the base frame so we can tweak brightness */
    frame = kmemdup(base_frame, frame_len, GFP_KERNEL);
    if (!frame)
        return;

    for (i = 4; i + 3 < frame_len - 4; i += 4) {
        u8 base_b = frame[i] & 0x1F;
        u8 bright = base_b;
        unsigned int step = anim_t % 62;

        switch (current_mode) {
        case ANIM_BLINK:
            bright = anim_state ? base_b : 0;
            break;
        case ANIM_FADE:
        case ANIM_PULSE:
            /* Triangle wave scaled to base brightness */
            {
                unsigned int triangle = (step < 31) ? step : (62 - step);
                bright = (base_b * triangle) / 31;
            }
            break;
        default:
            break;
        }

        frame[i] = 0xE0 | (bright & 0x1F);
    }

    spi_write(anim_spi, frame, frame_len);
    kfree(frame);

    anim_state = !anim_state;
    anim_t++;
    schedule_delayed_work(&anim_work, msecs_to_jiffies(delay_ms));
}

/* Initialize animation system (call from probe) */
int ledanim_init(struct spi_device *spi)
{
    anim_spi = spi;
    INIT_DELAYED_WORK(&anim_work, anim_worker);
    return 0;
}
EXPORT_SYMBOL(ledanim_init);

/* Stop any running animation (call from remove or before static color) */
void ledanim_stop(void)
{
    cancel_delayed_work_sync(&anim_work);
    kfree(base_frame);
    base_frame    = NULL;
    frame_len     = 0;
    current_mode  = ANIM_NONE;
    period_ms     = 0;
    anim_t        = 0;
    anim_state    = false;
}
EXPORT_SYMBOL(ledanim_stop);

/* Start a new animation */
int ledanim_start(enum anim_mode mode,
                  unsigned long period_ms_arg,
                  const u8 *frame,
                  size_t len)
{
    u8 *dup;
    unsigned long step_ms;

    /* Stop old */
    ledanim_stop();

    /* Copy base frame */
    dup = kmemdup(frame, len, GFP_KERNEL);
    if (!dup)
        return -ENOMEM;
    base_frame = dup;
    frame_len  = len;

    /* Compute per‐step delay */
    period_ms = period_ms_arg;  /* Store for getter */
    if (mode == ANIM_BLINK) {
        delay_ms = period_ms_arg;
    } else {
        step_ms = period_ms_arg / 62;
        if (!step_ms) step_ms = 1;
        delay_ms = step_ms;
    }

    current_mode = mode;
    anim_t       = 0;
    anim_state   = false;

    pr_info("ledanim: start mode=%d total=%lums step=%lums frame_len=%zu\n",
            mode, period_ms_arg, delay_ms, len);

    schedule_delayed_work(&anim_work, 0);
    return 0;
}
EXPORT_SYMBOL(ledanim_start);

/* Return true if an animation is currently running */
bool ledanim_is_active(void)
{
    return current_mode != ANIM_NONE;
}
EXPORT_SYMBOL(ledanim_is_active);

/* Swap in a new base frame while animating */
int ledanim_update_frame(const u8 *frame, size_t len)
{
    u8 *newbuf = kmemdup(frame, len, GFP_KERNEL);
    if (!newbuf)
        return -ENOMEM;
    kfree(base_frame);
    base_frame  = newbuf;
    frame_len   = len;
    return 0;
}
EXPORT_SYMBOL(ledanim_update_frame);

/* Get current animation mode */
enum anim_mode ledanim_get_mode(void)
{
    return current_mode;
}
EXPORT_SYMBOL(ledanim_get_mode);

/* Get current animation period */
unsigned long ledanim_get_period_ms(void)
{
    return period_ms;
}
EXPORT_SYMBOL(ledanim_get_period_ms);

MODULE_LICENSE("GPL");
