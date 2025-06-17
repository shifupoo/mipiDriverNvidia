# mipiDriverNvidia
This is a repository for creating the MIPI driver on NVIDIA Orin

# 1. Documentation and analize
   # Necessar information :
     About camera MIPI (example Sony IMX219) :
         -> Communication interface: I2C
         -> Stream format (RAW)
         -> Supply voltage (2.6 V; 2.8 V; 3.0 V);
         -> MIPI CSI-2 output: 2 data lanes ?
         -> Supported resolutions: up to 3280x2464
         -> Register map
     About NIVIDIA Orin :
         -> Documentation SOC
         -> Software platform : Jetson Linux /V4L2(Linux Tegra)
         -> Pinout CSI (Camera Serial Interface) + tegra-camss / VI (Video Input)
         -> V4L2 subdev framework

   # Source :
      - [Sony IMX219 Datashee] (https://www.opensourceinstruments.com/Electronics/Data/IMX219PQ.pdf)
      - [NVIDIA L4T Documentation ] (https://developer.nvidia.com/embedded/linux-tegra-r321)
      - [Linux Kernel Docs V4L2 ] ( https://www.kernel.org/doc/html/v4.8/media/kapi/v4l2-core.html)
      - [Mainline Linux]  (https://github.com/torvalds/linux/blob/master/drivers/media/i2c/imx290.c)
      - [Jetson Camera Bring-Up Wiki] ( https://docs.nvidia.com/jetson/archives/r35.4.1/DeveloperGuide/text/SD/CameraDevelopment.html)

  # Design and arhitecture 
      # Logical flow
    -----------------   ----------------------    -----------------------      ----------------------------------     ---------------    -------------------------  
     IMX219 (Camera) -> MIPI CSI (on Orin SOC) -> Video Input (VI) (Tegra ) -> V4L2 Subdivice (my_imx219.c driver) -> media controler -> user space (v4l2-ctl GStreamer)

     # Software componenets 
      -> driver v4l2 subdev 
      -> i2c client driver 
      -> divice tree
      -> media controller

# Integration Steps
   1. Confirm camera compatibility (resolution, lane config)
   2. Configure kernel and device tree
   3. Enable driver and platform support (via defconfig and Makefile)
   4. Boot kernel and inspect logs
   5. Validate user-space visibility

# Practical implementation see (my_imx219.c)
   -> a small implementation 
   ```
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
   ```

# Device Tree Example

      
