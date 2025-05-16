#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "ledparser.h"

struct named_color {
    const char *name;
    u8 r, g, b;
};

static struct named_color named_colors[] = {
    { "red",     255,   0,   0 },
    { "green",     0, 255,   0 },
    { "blue",      0,   0, 255 },
    { "yellow",  255, 255,   0 },
    { "cyan",      0, 255, 255 },
    { "magenta", 255,   0, 255 },
    { "white",   255, 255, 255 },
    { "black",     0,   0,   0 },
    { "orange",  255, 165,   0 },
    { "purple",  128,   0, 128 },
    { "pink",    255, 192, 203 },
    { NULL, 0, 0, 0 }
};

int parse_led_colors(const char *input, u8 **out_buf, size_t *out_len)
{
    char *buf, *tok, *saveptr;
    int led_count = 0;
    size_t i, max_leds = 32;
    u8 *spi_buf;

    buf = kstrdup(input, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    spi_buf = kzalloc(4 + max_leds * 4 + 4, GFP_KERNEL);
    if (!spi_buf) {
        kfree(buf);
        return -ENOMEM;
    }

    memset(spi_buf, 0x00, 4); // start frame

    tok = strtok_r(buf, " \t\n", &saveptr);
    while (tok && led_count < max_leds) {
        u8 r = 0, g = 0, b = 0, brightness = 31;

        if (tok[0] == '#' || strncmp(tok, "0x", 2) == 0) {
            u32 val = 0;
            char *colon = strchr(tok, ':');
            if (colon) {
                *colon = '\0';
                kstrtou8(colon + 1, 10, &brightness);
                if (brightness > 31) brightness = 31;
            }
            kstrtou32(tok + (tok[0] == '#' ? 1 : 2), 16, &val);
            r = (val >> 16) & 0xFF;
            g = (val >> 8) & 0xFF;
            b = val & 0xFF;
        } else {
            char *colon = strchr(tok, ':');
            if (colon) {
                *colon = '\0';
                kstrtou8(colon + 1, 10, &brightness);
                if (brightness > 31) brightness = 31;
            }
            for (i = 0; named_colors[i].name; i++) {
                if (strcmp(tok, named_colors[i].name) == 0) {
                    r = named_colors[i].r;
                    g = named_colors[i].g;
                    b = named_colors[i].b;
                    break;
                }
            }
        }

        i = 4 + led_count * 4;
        spi_buf[i + 0] = 0xE0 | (brightness & 0x1F);
        spi_buf[i + 1] = b;
        spi_buf[i + 2] = g;
        spi_buf[i + 3] = r;

        led_count++;
        tok = strtok_r(NULL, " \t\n", &saveptr);
    }

    memset(&spi_buf[4 + led_count * 4], 0xFF, 4); // end frame

    *out_buf = spi_buf;
    *out_len = 4 + led_count * 4 + 4;

    kfree(buf);
    return 0;
}
