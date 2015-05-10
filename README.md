# uefi_freertos_beagleboard

FreeRTOS application loader for UEFI on Beagle board

Please follow the instructions on [uefi with beagleboard on ubuntu using qemu](http://shortcircuits.io "shortcircuits uefi") post for instructions on how to setup qemu, edk2 and a few other tips.

Clone edk2 and copy the Include folder which contains headers files needed to compile freertos application.
``` shell-session
git clone git@github.com:tianocore/edk2.git
cp -r path-to-edk2/MdePkg/Include path-to-uefi_freertos_beagleboard/freertos_test/Demo/OMAP3_BeagleBoard_GCC
cd path-to-uefi_freertos_beagleboard/freertos_test/Demo/OMAP3_BeagleBoard_GCC
make
```
`rtosdemo.elf` file will be generated in the current directory.

Now to compile the osloader UEFI application.
``` shell-session
cp -r path-to-uefi_freertos_beagleboard/osloader path-to-edk2
cd path-to-edk2/BeagleBoardPkg
```

Edit  `BeagleBoardPkg.dsc` file  and add `osloader/osloader.inf` to the bottom of the file i.e. into `[Components.common]` section.

Below command will generate the needed `efi` file.
```
./build.sh -m ../osloader/osloader.inf
```

`osloader.efi` should be generated in `path-to-edk2/Build/BeagleBoard/DEBUG_ARMLINUXGCC/ARM/osloader/osloader/DEBUG/`

#### Links
1. [OS Loader](https://github.com/fgken/uefi-bootloader.git "os loader")


