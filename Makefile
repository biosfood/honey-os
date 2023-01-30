IMAGE_FILE = /run/user/1000/honey-os.img

CC = i686-elf-gcc
CCFLAGS = -m32 -mtune=generic -ffreestanding -nostdlib -c -I src/include -I src/kernel/include -Wno-discarded-qualifiers -fms-extensions -Wno-shift-count-overflow -O0
LD = i686-elf-ld
LD_FLAGS = -z max-page-size=0x1000 -T link.ld
AS = nasm
ASFlAGS = -felf32
EMU = qemu-system-x86_64
EMUFLAGS = -m 1G -drive format=raw,file=$(IMAGE_FILE) -no-reboot -no-shutdown -monitor stdio -d int -D crashlog.log -s -d int -device qemu-xhci -device usb-mouse

BUILD_FOLDER = build

SOURCE_FILES := $(shell find src/kernel -name *.c -or -name *.asm -or -name *.s)
OBJS := $(SOURCE_FILES:%=$(BUILD_FOLDER)/%.o)
USER_PROGRAMS := $(shell tree -d -L 1 -i --noreport src/userland/ | tail -n+2)
USER_PROGRAM_NAMES := $(USER_PROGRAMS:%=user/%)
USER_PROGRAM_FILES := $(USER_PROGRAMS:%=initrd/%)

run: $(USER_PROGRAM_NAMES) $(IMAGE_FILE)
	@echo "starting qemu"
	@$(EMU) $(EMUFLAGS)

$(IMAGE_FILE): rootfs/boot/kernel rootfs/initrd.tar
	@echo "creating the iso image"
	@dd if=/dev/zero of=$(IMAGE_FILE) bs=512 count=32768 &&\
	printf "n\np\n1\n\n\na\nw\n" | fdisk $(IMAGE_FILE) &&\
	loop0=$$(sudo losetup -f) &&\
	sudo losetup $$loop0 $(IMAGE_FILE) &&\
	loop1=$$(sudo losetup -f) &&\
	sudo losetup $$loop1 $(IMAGE_FILE) -o 1048576 &&\
	sudo mkfs.fat -F16 $$loop1 &&\
	sudo mount $$loop1 /mnt &&\
	sudo grub-install --root-directory=/mnt --no-floppy --modules="normal part_msdos multiboot" $$loop0 &&\
	sudo cp -RT rootfs/ /mnt &&\
	sync &&\
	sudo umount /mnt &&\
	sudo losetup -d $$loop0 &&\
	sudo losetup -d $$loop1

rootfs/initrd.tar: $(USER_PROGRAM_FILES)
	tar cvf rootfs/initrd.tar initrd/

rootfs/boot/kernel: $(OBJS) link.ld
	@echo "linking"
	@$(LD) $(LD_FLAGS) -o $@ $(OBJS)

$(BUILD_FOLDER)/%.asm.o: %.asm
	@echo "asembling $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFlAGS) $< -o $@

$(BUILD_FOLDER)/%.c.o: %.c
	@echo "compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CCFLAGS) -r $< -o $@

$(BUILD_FOLDER)/%.s.o: %.s
	@echo "assembling $<" 
	@mkdir -p $(dir $@)
	@$(CC) $(CCFLAGS) -r $< -o $@

user/%: src/userland/%
	@echo "compiling userspace program $<"
	@make -C $<

clean:
	@echo "clearing build folder"
	@rm -r $(BUILD_FOLDER)
