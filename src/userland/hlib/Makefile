CC = i686-elf-gcc
CCFLAGS = -m32 -mtune=generic -ffreestanding -nostdlib -c -I ../../include -I include -Wno-discarded-qualifiers -fms-extensions -Wno-shift-count-overflow -O0
LD = i686-elf-ld
LD_FLAGS = -z max-page-size=0x1000 
AS = nasm
ASFlAGS = -felf32

BUILD_FOLDER = build

SOURCE_FILES := $(shell find . -name "*.c" -or -name "*.asm")
OBJS := $(SOURCE_FILES:%=$(BUILD_FOLDER)/%.o)

../../../initrd/hlib: $(OBJS)
	@echo "linking the honey OS c libary"
	@$(LD) $(LD_FLAGS) -o ../../../build/hlib.o $(OBJS) -T link.ld -r
	@$(LD) $(LD_FLAGS) -o $@ $(OBJS) -T link.ld

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
