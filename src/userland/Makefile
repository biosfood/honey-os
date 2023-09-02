CC = i686-elf-gcc
CCFLAGS = -m32 -mtune=generic -ffreestanding -nostdlib -c -I ../include -Wno-discarded-qualifiers -fms-extensions -Wno-shift-count-overflow -O0 -Wno-div-by-zero
LD = i686-elf-ld
LD_FLAGS = -z max-page-size=0x1000 -T link.ld --no-warn-rwx-segment
AS = nasm
ASFlAGS = -felf32

SOURCE_FILES := $(shell find . -name "*.c" -or -name *.asm -or -name *.s)
OBJS := $(SOURCE_FILES:%=$(BUILD_FOLDER)/%.o)

PROGRAM_NAMES := $(shell ls -d */ | sed "s/\/$$//" | sed "s|build||")
PROGRAMS := $(PROGRAM_NAMES:%=../../initrd/%)

all: $(PROGRAMS)

.SUFFIXES:

define MAKE_PROGRAM

../../initrd/$(1): $(2)
	@echo "linking $(1) from $$^"
	@$(LD) $(LD_FLAGS) -o $$@ $$^

build/$(1)/%.c.o: $(1)/%.c
	@echo "compiling '$$<' to '$$@' for $(1)"
	@mkdir -p $$(dir $$@)
	@$(CC) $(CCFLAGS) $$< -o $$@ -I $(1)/include

endef

$(foreach NAME, $(PROGRAM_NAMES), $(eval $(call MAKE_PROGRAM,$(NAME),$(shell find $(NAME) -name "*.c" | sed "s|$$|.o|" | sed "s|^|build/|"))))


#  $(shell find $(subst user/,,$@) -name "*.c" | sed "s?/\([^/]*\)?/build/\1?")
#user/%: $$(shell echo "test")
#	NAME=$$(echo $@ | sed "s?user/??");  \
#	SOURCES=$$(find $$NAME -name "*.c"); \
#	OBJS=$$(echo $$SOURCES | sed "s|$$|.o|" | sed "s|^|build/|"); \
#	make $$OBJS;\
#	echo "linking $@ $<";\
#	$(LD) $(LD_FLAGS) -I include$$NAME -o ../../initrd/$$NAME 
