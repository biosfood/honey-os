IMAGE_FILE = /run/user/1000/tree-os.img

CC = i686-elf-gcc
CCFLAGS = -m32 -mtune=generic -ffreestanding -nostdlib -c -I src/include -I src/kernel/include -Wno-discarded-qualifiers -fms-extensions -Wno-shift-count-overflow -O0
LD = i686-elf-ld
LD_FLAGS = -z max-page-size=0x1000 -T link.ld
AS = nasm
ASFlAGS = -felf32
GENISO = genisoimage
GENISOFLAGS = -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -input-charset utf8 -quiet -boot-info-table -A tree-os
EMU = qemu-system-x86_64
EMUFLAGS = -m 1G -drive format=raw,file=$(IMAGE_FILE) -no-reboot -no-shutdown -monitor stdio

BUILD_FOLDER = build

SOURCE_FILES := $(shell find src/kernel -name *.c -or -name *.asm -or -name *.s)
OBJS := $(SOURCE_FILES:%=$(BUILD_FOLDER)/%.o)
USER_PROGRAMS := $(shell ls src/userland)
USER_PROGRAM_NAMES := $(USER_PROGRAMS:%=rootfs/%)

run: $(IMAGE_FILE)
	@echo "starting qemu"
	@$(EMU) $(EMUFLAGS)

$(IMAGE_FILE): rootfs/boot/kernel $(USER_PROGRAM_NAMES)
	echo "creating the iso image"
	dd if=/dev/zero of=$(IMAGE_FILE) bs=512 count=32768 &&\
	printf "n\np\n1\n\n\na\nw\n" | fdisk $(IMAGE_FILE) &&\
	loop0=$$(sudo losetup -f) &&\
	sudo losetup $$loop0 $(IMAGE_FILE) &&\
	loop1=$$(sudo losetup -f) &&\
	sudo losetup $$loop1 $(IMAGE_FILE) -o 1048576 &&\
	sudo mkfs.fat -F16 $$loop1 &&\
	sudo mount $$loop1 /mnt &&\
	sudo grub-install --root-directory=/mnt --no-floppy --modules="normal part_msdos multiboot" $$loop0 &&\
	sudo cp -RT rootfs/ /mnt &&\
	sudo rm -r /mnt/boot/grub/fonts &&\
	sync &&\
	sudo umount /mnt &&\
	sudo losetup -d $$loop0 &&\
	sudo losetup -d $$loop1

rootfs/boot/kernel: $(OBJS)
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

rootfs/%: src/userland/% iso/modules
	@echo "compiling userspace program $<"
	@make -C $<

clean:
	@echo "clearing build folder"
	@rm -r $(BUILD_FOLDER) tree-os.iso

cleanELF:
	@echo "clearing the elf file"
	@rm iso/boot/tree-os.elf
