#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include "ledanim.h"

static struct delayed_work anim_work;
enum anim_mode current_mode = ANIM_NONE;
static struct spi_device *anim_spi;
static u8 *base_frame;
static size_t frame_len;
static unsigned long delay_ms;
static bool state = false;
static unsigned int t = 0;

static void anim_worker(struct work_struct *work)
{
    //pr_info("anim_worker: frame_len=%zu\n", frame_len);

    if (!base_frame || !anim_spi || frame_len < 12) {
        pr_err("musedacled: animation worker abort (invalid buffer)\n");
        return;
    }

    u8 *frame = kzalloc(frame_len, GFP_KERNEL);
    if (!frame) return;
    memcpy(frame, base_frame, frame_len);

    for (size_t i = 4; i < frame_len - 4; i += 4) {
        u8 base = base_frame[i] & 0x1F;
        u8 brightness = base;

        switch (current_mode) {
            case ANIM_BLINK:
                brightness = state ? base : 0;
                break;
            case ANIM_FADE:
                brightness = 16 + (15 * abs(31 - (t % 62))) / 31;
                break;
            case ANIM_PULSE:
                brightness = (t % 62 < 31) ? (t % 31) : (62 - (t % 62));
                break;
            default:
                break;
        }

        frame[i] = 0xE0 | (brightness & 0x1F);
    }

    spi_write(anim_spi, frame, frame_len);
    kfree(frame);

    state = !state;
    t++;
    schedule_delayed_work(&anim_work, msecs_to_jiffies(delay_ms));
}

int ledanim_init(struct spi_device *spi)
{
    anim_spi = spi;
    INIT_DELAYED_WORK(&anim_work, anim_worker);
    return 0;
}

void ledanim_stop(void)
{
    cancel_delayed_work_sync(&anim_work);
    kfree(base_frame);
    base_frame = NULL;
    current_mode = ANIM_NONE;
    t = 0;
    state = false;
}

int ledanim_start(enum anim_mode mode, unsigned long d_ms, const u8 *frame, size_t len)
{
    pr_info("musedacled: starting anim mode %d, delay %lu, frame_len %zu\n", mode, d_ms, len);

    ledanim_stop();

    base_frame = kmemdup(frame, len, GFP_KERNEL);
    if (!base_frame)
        return -ENOMEM;

    frame_len = len;

    current_mode = mode;
    delay_ms = d_ms;
    schedule_delayed_work(&anim_work, 0);
    return 0;
}

bool ledanim_is_active(void)
{
    return (current_mode != ANIM_NONE);
}

int ledanim_update_frame(const u8 *frame, size_t len)
{
    u8 *new = kmemdup(frame, len, GFP_KERNEL);
    if (!new) return -ENOMEM;
    kfree(base_frame);
    base_frame = new;
    frame_len  = len;
    return 0;
}


EXPORT_SYMBOL(ledanim_is_active);
EXPORT_SYMBOL(ledanim_init);
EXPORT_SYMBOL(ledanim_start);
EXPORT_SYMBOL(ledanim_stop);
EXPORT_SYMBOL(ledanim_update_frame);

MODULE_LICENSE("GPL");
