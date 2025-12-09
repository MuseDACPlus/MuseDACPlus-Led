#include <linux/device/class.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include "ledparser.h"
#include "ledanim.h"

#define DEVICE_NAME "musedacled"
#define CLASS_NAME  "musedac"
#define DRIVER_VERSION "1.0.12"

static int major;
static struct class *musedacled_class;
static struct cdev musedacled_cdev;
static struct spi_device *spi_dev;
static u8 *last_color_frame;
static size_t last_color_frame_len;

// SPI driver probe/remove declarations
static int musedacled_probe(struct spi_device *spi);
static void musedacled_remove(struct spi_device *spi);

// Device Tree match
static const struct of_device_id musedacled_dt_ids[] = {
    { .compatible = "musedac,led" },
    { }
};
MODULE_DEVICE_TABLE(of, musedacled_dt_ids);

static const struct spi_device_id musedacled_id[] = {
    { "musedacled", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, musedacled_id);

static struct spi_driver musedacled_driver = {
    .driver = {
        .name = DEVICE_NAME,
        .of_match_table = musedacled_dt_ids,
    },
    .probe = musedacled_probe,
    .remove = musedacled_remove,
    .id_table = musedacled_id,
};

static ssize_t musedacled_write(struct file *file,
                                const char __user *buf,
                                size_t len,
                                loff_t *offset)
{
    char *kbuf, *arg, *mode_str;
    ssize_t ret = len;

    if (!spi_dev)
        return -ENODEV;

    kbuf = kmalloc(len+1, GFP_KERNEL);
    if (!kbuf) return -ENOMEM;
    if (copy_from_user(kbuf, buf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }
    kbuf[len] = '\0';
    arg = strim(kbuf);

    /* STOP command */
    if (!strncasecmp(arg, "stop", 4) && arg[4]=='\0') {
        pr_info("musedacled: stopping animation\n");
        ledanim_stop();
        goto out;
    }

    /* COLOR command */
    if (!strncasecmp(arg, "color", 5)) {
        u8 *spi_buf;
        size_t spi_len;
        char *color_arg = strim(arg + 5);

        if (!*color_arg) {
            pr_err("musedacled: no color specified\n");
            ret = -EINVAL; goto out;
        }

        if (parse_led_colors(color_arg, &spi_buf, &spi_len) < 0) {
            pr_err("musedacled: failed to parse color '%s'\n", color_arg);
            ret = -EINVAL; goto out;
        }

        /* store the new static frame */
        kfree(last_color_frame);
        last_color_frame = kmemdup(spi_buf, spi_len, GFP_KERNEL);
        last_color_frame_len = spi_len;

        /* if an animation is running, just update its base frame */
        if (ledanim_is_active()) {
            ledanim_update_frame(last_color_frame, last_color_frame_len);
        } else {
            /* otherwise, send it statically */
            spi_write(spi_dev, spi_buf, spi_len);
        }

        kfree(spi_buf);
        goto out;
    }

    /* ANIMATION command */
    if (!strncasecmp(arg, "anim", 4)) {
        unsigned long delay;
        enum anim_mode mode;
        char *anim_arg = strim(arg + 4);

        mode_str = strsep(&anim_arg, ":");
        if (!mode_str || !anim_arg) {
            pr_err("musedacled: anim format must be <type>:<delay>\n");
            ret = -EINVAL; goto out;
        }
        if (kstrtoul(anim_arg, 10, &delay)) {
            pr_err("musedacled: invalid delay '%s'\n", anim_arg);
            ret = -EINVAL; goto out;
        }
        if (!strcasecmp(mode_str, "blink"))      mode = ANIM_BLINK;
        else if (!strcasecmp(mode_str, "fade"))  mode = ANIM_FADE;
        else if (!strcasecmp(mode_str, "pulse")) mode = ANIM_PULSE;
        else { pr_err("musedacled: unknown anim '%s'\n", mode_str);
            ret = -EINVAL; goto out;
        }
        if (!last_color_frame) {
            pr_err("musedacled: set a color before anim\n");
            ret = -EINVAL; goto out;
        }
        pr_info("musedacled: starting anim %s:%lu (frame_len=%zu)\n",
                mode_str, delay, last_color_frame_len);
        ledanim_start(mode, delay,
                      last_color_frame, last_color_frame_len);
        goto out;
    }

    pr_err("musedacled: unrecognized command '%s'\n", arg);
    ret = -EINVAL;

out:
    kfree(kbuf);
    return ret;
}

static ssize_t musedacled_read(struct file *file,
                               char __user *buf,
                               size_t len,
                               loff_t *offset)
{
    char *status;
    size_t status_len;
    ssize_t ret;
    enum anim_mode mode;
    unsigned long period;
    const char *mode_str;

    /* Only read once */
    if (*offset > 0)
        return 0;

    mode = ledanim_get_mode();
    period = ledanim_get_period_ms();

    switch (mode) {
    case ANIM_BLINK: mode_str = "blink"; break;
    case ANIM_FADE:  mode_str = "fade";  break;
    case ANIM_PULSE: mode_str = "pulse"; break;
    default:         mode_str = "none";  break;
    }

    status = kasprintf(GFP_KERNEL,
        "MuseDAC+ LED Driver v%s\n"
        "\n"
        "Current Status:\n"
        "  Animation: %s\n"
        "  Period:    %lu ms\n"
        "  LEDs:      %zu\n"
        "\n"
        "Available Commands:\n"
        "  color <spec>           - Set LED colors\n"
        "    Named colors:        red, green, blue, yellow, cyan, magenta, white, black\n"
        "    Hex colors:          #RRGGBB or 0xRRGGBB\n"
        "    With brightness:     <color>:0-31 (e.g., red:20)\n"
        "    Multiple LEDs:       color red green blue\n"
        "    Example:             echo -n 'color red:15 green:10 blue:5' > /dev/musedacled\n"
        "\n"
        "  anim blink:<ms>        - Blink animation\n"
        "    Example:             echo -n 'anim blink:300' > /dev/musedacled\n"
        "\n"
        "  anim fade:<ms>         - Smooth fade animation\n"
        "    Example:             echo -n 'anim fade:1000' > /dev/musedacled\n"
        "\n"
        "  anim pulse:<ms>        - Linear pulse animation\n"
        "    Example:             echo -n 'anim pulse:500' > /dev/musedacled\n"
        "\n"
        "  stop                   - Stop current animation\n"
        "    Example:             echo -n 'stop' > /dev/musedacled\n"
        "\n"
        "Note: Always use 'echo -n' or 'printf' to avoid trailing newlines\n",
        DRIVER_VERSION,
        mode_str,
        period,
        last_color_frame ? ((last_color_frame_len - 8) / 4) : 0
    );

    if (!status)
        return -ENOMEM;

    status_len = strlen(status);
    ret = simple_read_from_buffer(buf, len, offset, status, status_len);
    kfree(status);

    return ret;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read  = musedacled_read,
    .write = musedacled_write,
};

static int musedacled_probe(struct spi_device *spi)
{
    dev_info(&spi->dev, "musedacled loaded — version %s\n", DRIVER_VERSION);

    dev_t dev;
    int ret;

    spi_dev = spi;
    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    spi->max_speed_hz = 8000000;
    spi_setup(spi);

    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    major = MAJOR(dev);
    cdev_init(&musedacled_cdev, &fops);
    cdev_add(&musedacled_cdev, dev, 1);

    musedacled_class = class_create(CLASS_NAME);
    device_create(musedacled_class, NULL, dev, NULL, DEVICE_NAME);

    ledanim_init(spi);

    pr_info("musedacled: bound to SPI and ready\n");
    return 0;
}

static void musedacled_remove(struct spi_device *spi)
{
    dev_t dev = MKDEV(major, 0);

    ledanim_stop();

    device_destroy(musedacled_class, dev);
    class_destroy(musedacled_class);
    cdev_del(&musedacled_cdev);
    unregister_chrdev_region(dev, 1);
}

static int __init musedacled_init(void)
{
    return spi_register_driver(&musedacled_driver);
}

static void __exit musedacled_exit(void)
{
    spi_unregister_driver(&musedacled_driver);
}

module_init(musedacled_init);
module_exit(musedacled_exit);

MODULE_AUTHOR("Vincent Saydam <vincent@wickedcreations.nl>");
MODULE_DESCRIPTION("SPI LED driver with animations for MuseDAC+ HAT");
MODULE_LICENSE("GPL");
