#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include "ledparser.h"

#define MAX_LEDS 32

struct named_color {
    const char *name;
    u8 r, g, b;
};

static const struct named_color named_colors[] = {
    { "red",     255,   0,   0 },
    { "green",     0, 255,   0 },
    { "blue",      0,   0, 255 },
    { "yellow",  255, 255,   0 },
    { "cyan",      0, 255, 255 },
    { "magenta", 255,   0, 255 },
    { "white",   255, 255, 255 },
    { "black",     0,   0,   0 },
    { NULL,        0,   0,   0 }
};

/**
 * parse_led_colors()  
 * @input:   e.g. "red green:10 #00ff00:5"  (no leading/trailing whitespace)  
 * @out_buf: pointer to newly-kmalloc’d SPI frame (start+LEDs+end)  
 * @out_len: length of that buffer  
 *
 * Returns 0 on success, <0 on error.
 */
int parse_led_colors(const char *input, u8 **out_buf, size_t *out_len)
{
    char *buf, *tok, *saveptr;
    size_t led_count = 0;
    u8 *spi;

    if (!input || !*input)
        return -EINVAL;

    buf = kstrdup(input, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    /* Allocate start(4) + MAX_LEDS×4 + end(4) */
    spi = kzalloc(4 + MAX_LEDS * 4 + 4, GFP_KERNEL);
    if (!spi) {
        kfree(buf);
        return -ENOMEM;
    }

    /* start frame */
    memset(spi, 0x00, 4);

    /* tokenize on spaces/tabs/newlines */
    saveptr = buf;
    while ((tok = strsep(&saveptr, " \t\n")) && led_count < MAX_LEDS) {
        u8 r = 0, g = 0, b = 0, brightness = 31;
        char *c, *val;
        u32 hex;

        if (!*tok)
            continue;   /* skip empty tokens */

        /* handle brightness suffix "color:10" */
        c = strchr(tok, ':');
        if (c) {
            *c++ = '\0';
            if (kstrtou8(c, 10, &brightness) || brightness > 31)
                brightness = 31;
        }

        /* hex notation */
        if (tok[0] == '#' || (tok[0]=='0' && tok[1]=='x')) {
            val = (tok[0]=='#') ? tok+1 : tok+2;
            if (kstrtou32(val, 16, &hex))
                goto next_tok;
            r = (hex >> 16) & 0xFF;
            g = (hex >> 8)  & 0xFF;
            b = hex & 0xFF;
        }
        /* named color */
        else {
            size_t i;
            for (i = 0; named_colors[i].name; i++) {
                if (!strcasecmp(tok, named_colors[i].name)) {
                    r = named_colors[i].r;
                    g = named_colors[i].g;
                    b = named_colors[i].b;
                    break;
                }
            }
            if (!named_colors[i].name)
                goto next_tok;  /* unknown name */
        }

        /* write LED frame: [E0|b][B][G][R] */
        {
            size_t ofs = 4 + led_count*4;
            spi[ofs+0] = 0xE0 | (brightness & 0x1F);
            spi[ofs+1] = b;
            spi[ofs+2] = g;
            spi[ofs+3] = r;
            led_count++;
        }

    next_tok:
        continue;
    }

    /* end frame: 4 bytes of 0xFF */
    memset(spi + 4 + led_count * 4, 0xFF, 4);

    kfree(buf);
    *out_buf = spi;
    *out_len = 4 + led_count*4 + 4;

    /* we require at least one LED */
    return (led_count > 0) ? 0 : -EINVAL;
}
EXPORT_SYMBOL(parse_led_colors);
MODULE_LICENSE("GPL");
