#ifndef LEDPARSER_H
#define LEDPARSER_H

#include <linux/types.h>
#include <linux/slab.h>

/**
 * Parses a text-based LED color string into an SPI buffer.
 *
 * @param input     Null-terminated string with color tokens (e.g. "red green #FF00FF:10")
 * @param out_buf   Pointer to allocated SPI data buffer (caller must kfree)
 * @param out_len   Length of the buffer in bytes
 * @return          0 on success, -ENOMEM or -EINVAL on error
 */
int parse_led_colors(const char *input, u8 **out_buf, size_t *out_len);

#endif // LEDPARSER_H
