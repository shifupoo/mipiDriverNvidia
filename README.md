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

  # 2. Design and arhitecture 
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

# 3. Practical implementation see (my_imx219.c)
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
```
&i2c0 {
    imx219_a@10 {
        compatible = "sony,imx219";
        reg = <0x10>;
        clocks = <&bpmp TEGRA234_CLK_CAM0>;
        clock-names = "xclk";
        reset-gpios = <&gpio CAM0_RST GPIO_ACTIVE_LOW>;

        port {
            imx219_out0: endpoint {
                remote-endpoint = <&csi_in0>;
                bus-width = <2>;
                data-lanes = <1 2>;
                clock-noncontinuous;
                link-frequencies = /bits/ 64 <456000000>;
            };
        };
    };
};

&vi {
    ports {
        port@0 {
            csi_in0: endpoint {
                remote-endpoint = <&imx219_out0>;
                bus-width = <2>;
                data-lanes = <1 2>;
            };
        };
    };
};
```

# Kernel integration
```
   -> Add driver to drivers/media/i2c/Makefile and Kconfig 
   -> Build kernel and device tree
   -> test it on Orin (Not possible for me :()
```

# 4. Test and debug
```
   -> I used a Linux jetson-dev 5.15.0-141-generic x86_64
   -> i2cdetect -y 0
   -> dmesg | grep imx219
   -> v4l2-ctl --list-devices
   -> v4l2-ctl --device /dev/video0 --stream-mmap --stream-count=1 --stream-to=frame.raw
```
# Logs to watch 
      -> dmesg
      -> /dev/video* creation
      -> I2C register reads and chip ID detection

# MIPI Signal Check 
      -> Use oscilloscope or logic analyzer on D0, D1, CLK (I hope I will have the opportunity)

 # 5. Integration with GStreamer / DeepStream
     # GStreamer Pipeline
      -> gst-launch-1.0 v4l2src device=/dev/video0 ! 'video/x-raw,width=1280,height=720,framerate=30/1' ! autovideosink

      # DeepStream Configuration (deepstream_app_config.txt)
   ```
   [source0]
   enable=1
   type=1
   camera-width=1280
   camera-height=720
   camera-fps-n=30
   camera-fps-d=1
   device=/dev/video0
```
      # Troubleshooting
      -> Check gst-inspect-1.0 v4l2src
      -> Review /var/log/syslog or journalctl

# 6. Test Plan and Troubleshooting
   1. compile the driver
   2. i2c detection i2cdetect -y 0
   3. dmesg | grep my_imx219
   4. v4l2 listing v4l2-ctl --list-devices
   5. capture the test frame v4l2-ctl --stream-to=frame.raw
   6. Pipeline with GStreamer gst-launch-1.0

 # Commun issues 
    1. No /dev/video0: Check DT bindings, sensor not probed
    2. No image : check the resolution or format 
    
