CC = i686-elf-gcc
CCFLAGS = -m32 -mtune=generic -ffreestanding -nostdlib -c -I ../../include -I include -Wno-discarded-qualifiers -fms-extensions -Wno-shift-count-overflow -O0
LD = i686-elf-ld
LD_FLAGS = -z max-page-size=0x1000 -e main
AS = nasm
ASFlAGS = -felf32

BUILD_FOLDER = build

SOURCE_FILES := $(shell find . -name *.c -or -name *.asm -or -name *.s)
OBJS := $(SOURCE_FILES:%=$(BUILD_FOLDER)/%.o)

../../../initrd/pic: $(OBJS)
	@echo "linking user program 8259 PIC"
	@$(LD) $(LD_FLAGS) -o ../../../initrd/pic $(OBJS)

$(BUILD_FOLDER)/%.asm.o: %.asm
	@echo "asembling $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFlAGS) $< -o $@

$(BUILD_FOLDER)/%.c.o: %.c
	@echo "compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CCFLAGS) -r $< -o $@

clean:
	@echo "clearing build folder"
	@rm -r $(BUILD_FOLDER)
