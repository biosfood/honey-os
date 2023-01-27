# Honey OS

![](honey-os.png)

Honey os is an operating system developed by me. Its main goals consist of learning about x86 architecture and os development as well as ensuring good readability and easily understood code.

## Building

To build the OS and run in an emulator, run

    make

## Directory structure

The `rootfs` directory contains all files for the 'root file system' of the final iso. It contains the final honey os kernel executable file as well as necessary grub configuration files.

The `initrd` directory will be created during the building process as a temporary directory. This is later packed into a `.tar` file and will be loaded by grub. After booting, the kernel will be able to load additional user mode programs from this 'initial ramdisk', before other file system drivers have been initialized to load programs from the boot drive.

`src/kernel` contains all files which are compiled into the finished kernel.

`src/userland` contains several directories with source files to compile different ELF user programs.

`src/userland/hlib` is the honey OS utility library. It is linked by default with all loaded user mode programs at a high memory address (0xFF000000). And can be linked with and assumed to be loaded by any other program, because it will be loaded first.
