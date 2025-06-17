# mipiDriverNvidia
This is a repository for creating the MIPI driver on NVIDIA Orin

# 1. Documentation and analize
   # Necessar information :
     About camera MIPI (example Sony IMX219) :
         -> datasheet complet
         -> stream format (RAW)
         -> supply voltage (2.6 V; 2.8 V; 3.0 V);
         -> mipi transmitter
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


      
