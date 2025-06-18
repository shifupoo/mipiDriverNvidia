// SPDX-License-Identifier: GNU General Public License version 2
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/media-device.h>

#define IMX219_ID          0x0219
#define REG_CHIP_ID_HIGH   0x0000
#define REG_CHIP_ID_LOW    0x0001

/*structure for v4l2 subdivice*/
struct imx219 {
    struct v4l2_subdev sd;           // Subdevice V4L2
    struct media_pad pad;            // Media pad for links
    struct v4l2_ctrl_handler ctrl_handler;  // Control handler (brightness, gain, etc.)
};

static int imx219_s_stream(struct v4l2_subdev *sd, int enable)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    dev_info(&client->dev, "s_stream: %s\n", enable ? "START" : "STOP");

    // Start/Stop streaming

    return 0;
}

static int imx219_set_fmt(struct v4l2_subdev *sd,
                          struct v4l2_subdev_state *sd_state,
                          struct v4l2_subdev_format *fmt)
{
    fmt->format.width = 3280;
    fmt->format.height = 2464;
    fmt->format.code = MEDIA_BUS_FMT_SRGGB10_1X10; // RAW10
    fmt->format.field = V4L2_FIELD_NONE;

    return 0;
}

static int imx219_get_fmt(struct v4l2_subdev *sd,
                          struct v4l2_subdev_state *sd_state,
                          struct v4l2_subdev_format *fmt)
{
    // Returnează același format hardcodat
    fmt->format.width = 3280;
    fmt->format.height = 2464;
    fmt->format.code = MEDIA_BUS_FMT_SRGGB10_1X10;
    fmt->format.field = V4L2_FIELD_NONE;

    return 0;
}

static const struct v4l2_subdev_core_ops imx219_core_ops = {
    .log_status = v4l2_ctrl_subdev_log_status,
};

static const struct v4l2_subdev_video_ops imx219_video_ops = {
    .s_stream = imx219_s_stream,
};

static const struct v4l2_subdev_pad_ops imx219_pad_ops = {
    .set_fmt = imx219_set_fmt,
    .get_fmt = imx219_get_fmt,
};


static const struct v4l2_subdev_ops imx219_subdev_ops = {
    .core  = &imx219_core_ops,   // (log_status, etc.)
    .video = &imx219_video_ops,  // streaming on/off
    .pad   = &imx219_pad_ops,    // format video, etc.
};


/*
 * probe() – the function is called automaticaly by the Kernel
 * when a compatible divice show up in the Divice tree
 */
static int imx219_probe(struct i2c_client *client)
{
    int high, low, chip_id;
    struct imx219 *sensor;
    int ret;

    dev_info(&client->dev, "IMX219 probe() called\n");
    
    // Alloc memory for the strcuture of the sensor
    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    // Initialize the v4l2_subdev 
    v4l2_i2c_subdev_init(&sensor->sd, client, &imx219_subdev_ops);
    
    // Read register 0x0000 (MODEL_ID[15:8]) 
    high = i2c_smbus_read_byte_data(client, REG_CHIP_ID_HIGH);
    // Read register 0x0001 (MODEL_ID[7:0])
    low  = i2c_smbus_read_byte_data(client, REG_CHIP_ID_LOW);

    if (high < 0 || low < 0) {
        dev_err(&client->dev, "Failed to read chip ID\n");
        return -EIO;
    }

    chip_id = (high << 8) | low;
    dev_info(&client->dev, "IMX219 chip ID read: 0x%04x\n", chip_id);

    if (chip_id != IMX219_ID) {
        dev_err(&client->dev, "Unexpected chip ID: 0x%04x\n", chip_id);
        return -ENODEV;
    }

    dev_info(&client->dev, "IMX219 sensor detected successfully!\n");
    
    /* Initialize subdev */
    sensor->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;

    /* Initialize source pad */
    sensor->pad.flags = MEDIA_PAD_FL_SOURCE;

    ret = media_entity_pads_init(&sensor->sd.entity, 1, &sensor->pad);
    if (ret < 0) {
        dev_err(&client->dev, "failed to init entity pads: %d\n", ret);
        return ret;
    }
    
    ret = v4l2_async_register_subdev(&sensor->sd);
    if (ret) {
        dev_err(&client->dev, "Failed to register subdevice\n");
        return ret;
    }

    return 0;
}

static int imx219_remove(struct i2c_client *client)
{
    struct v4l2_subdev *sd = i2c_get_clientdata(client);
    dev_info(&client->dev, "IMX219 remove() called\n");

    v4l2_async_unregister_subdev(sd);
    media_entity_cleanup(&sd->entity);

    return 0;
}

/*
 * Device Tree binding – define compatibility
 * It has to be DTS un node with:
 *   compatible = "sony,imx219";
 */
static const struct of_device_id imx219_of_match[] = {
    { .compatible = "sony,imx219" },
    { /* sentinel */ }
};

/*Export the imx219_of_match into the .ko file*/
MODULE_DEVICE_TABLE(of, imx219_of_match);

/*
 * i2c_driver – register the I2C driver
 * We use .probe_new for Tegra-style
 */
static struct i2c_driver imx219_i2c_driver = {
    .driver = {
        .name = "imx219",
        .of_match_table = imx219_of_match,
    },
    .probe_new = imx219_probe,
    .remove    = imx219_remove,
};

/*
 * Automaticaly register the i2c driver in to the Kernel
 * (what module_init() and module_exit() is doing)
*/
module_i2c_driver(imx219_i2c_driver);

MODULE_AUTHOR("Muraru Mihaela");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sony IMX219 V4L2 sensor driver");

