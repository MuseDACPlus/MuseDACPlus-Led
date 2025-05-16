#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include "ledparser.h"

#define DEVICE_NAME "musedacled"
#define CLASS_NAME  "musedac"

static int major;
static struct class *musedacled_class;
static struct cdev musedacled_cdev;
static struct spi_device *spi_dev;

// SPI driver probe and remove
static int musedacled_probe(struct spi_device *spi);
static void musedacled_remove(struct spi_device *spi);

// Device Tree match table
static const struct of_device_id musedacled_dt_ids[] = {
    { .compatible = "musedac,led" },
    { }
};
MODULE_DEVICE_TABLE(of, musedacled_dt_ids);

// SPI device ID table
static const struct spi_device_id musedacled_id[] = {
    { "musedacled", 0 },
    { "musedac,led", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, musedacled_id);

// SPI driver definition
static struct spi_driver musedacled_driver = {
    .driver = {
        .name           = DEVICE_NAME,
        .of_match_table = musedacled_dt_ids,
    },
    .probe    = musedacled_probe,
    .remove   = musedacled_remove,
    .id_table = musedacled_id,
};

static int __init musedacled_init_driver(void)
{
    return spi_register_driver(&musedacled_driver);
}

static void __exit musedacled_exit_driver(void)
{
    spi_unregister_driver(&musedacled_driver);
}

// File operations
static int musedacled_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int musedacled_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t musedacled_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    u8 *kbuf;
    ssize_t ret;

    if (!spi_dev)
        return -ENODEV;

    // Allocate +1 for null terminator
    kbuf = kmalloc(len + 1, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }
    kbuf[len] = '\0';

    if (isprint(kbuf[0])) {
        u8 *spi_buf;
        size_t spi_len;

        if (parse_led_colors(kbuf, &spi_buf, &spi_len) == 0) {
            ret = spi_write(spi_dev, spi_buf, spi_len);
            kfree(spi_buf);

            if (ret == 0)
                ret = len; // Success: report full user write length
        } else {
            pr_warn("musedacled: invalid color string: %s\n", kbuf);
            ret = -EINVAL;
        }
    } else {
        ret = spi_write(spi_dev, kbuf, len);
        if (ret == 0)
            ret = len;
    }

    kfree(kbuf);
    return ret;
}


static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = musedacled_open,
    .release = musedacled_release,
    .write = musedacled_write,
};

static char *musedacled_devnode(const struct device *dev, umode_t *mode)
{
    if (mode)
        *mode = 0666;  // or 0660 for group-based access
    return NULL;
}

// SPI probe: sets up char device
static int musedacled_probe(struct spi_device *spi)
{
    dev_t dev;
    int ret;

    spi_dev = spi;

    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    spi->max_speed_hz = 8000000;
    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "spi_setup failed: %d\n", ret);
        return ret;
    }

    ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    major = MAJOR(dev);
    cdev_init(&musedacled_cdev, &fops);
    ret = cdev_add(&musedacled_cdev, dev, 1);
    if (ret < 0)
        return ret;

    musedacled_class = class_create(CLASS_NAME);
    if (IS_ERR(musedacled_class))
        return PTR_ERR(musedacled_class);

    musedacled_class->devnode = musedacled_devnode;

    device_create(musedacled_class, NULL, dev, NULL, DEVICE_NAME);
    dev_info(&spi->dev, "musedacled: bound to SPI5\n");
    return 0;
}

// SPI remove: cleans up char device
static void musedacled_remove(struct spi_device *spi)
{
    dev_t dev = MKDEV(major, 0);

    device_destroy(musedacled_class, dev);
    class_destroy(musedacled_class);
    cdev_del(&musedacled_cdev);
    unregister_chrdev_region(dev, 1);
}

module_init(musedacled_init_driver);
module_exit(musedacled_exit_driver);

MODULE_AUTHOR("Vincent Saydam <vincent@wickedcreations.nl>");
MODULE_DESCRIPTION("SPI5-based LED driver for musedacled HAT");
MODULE_LICENSE("GPL");

