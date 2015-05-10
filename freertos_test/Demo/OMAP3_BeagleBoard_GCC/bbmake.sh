make clean 
make ARCH=arm CROSS_COMPILE=arm-none-eabi- && rm /media/dshetty/boot/rtosdemo.bin && sudo cp rtosdemo.bin /media/dshetty/boot/ && sudo umount /media/dshetty/boot && sudo umount /media/dshetty/Narcissus-rootfs && sudo minicom
