#ifndef LEDPARSER_H
#define LEDPARSER_H

#include <linux/types.h>

int parse_led_colors(const char *input, u8 **out_buf, size_t *out_len);

#endif
