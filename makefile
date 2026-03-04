CXX = g++
AS  = as
LD  = ld

SRC_DIRS = src/arch/i386 src/kernel src/memory src/drivers src/fs src/net src/ui src/user src/common
INCLUDE_DIRS = $(addprefix -I,$(SRC_DIRS))

GPPPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore $(INCLUDE_DIRS)
ASPARAMS = --32
LDPARAMS = -melf_i386
MKRESCUE = $(shell command -v grub2-mkrescue 2>/dev/null || command -v grub-mkrescue 2>/dev/null)

OBJDIR = build/obj
ISODIR = build/iso

VPATH = $(SRC_DIRS)

objects = $(OBJDIR)/loader.o $(OBJDIR)/kernel.o $(OBJDIR)/gdt.o $(OBJDIR)/gdt_asm.o \
          $(OBJDIR)/interrupts.o $(OBJDIR)/interrupts_asm.o \
          $(OBJDIR)/keyboard.o $(OBJDIR)/mouse.o $(OBJDIR)/pci.o \
          $(OBJDIR)/memorymanagement.o $(OBJDIR)/allocator.o \
          $(OBJDIR)/vga.o $(OBJDIR)/driver.o $(OBJDIR)/gui.o \
          $(OBJDIR)/ata.o $(OBJDIR)/mbr.o $(OBJDIR)/fat32.o \
          $(OBJDIR)/ethernet.o $(OBJDIR)/arp.o $(OBJDIR)/ipv4.o \
          $(OBJDIR)/icmp.o $(OBJDIR)/udp.o $(OBJDIR)/tcp.o \
          $(OBJDIR)/rtl8139.o $(OBJDIR)/tss.o $(OBJDIR)/syscall.o \
          $(OBJDIR)/process.o $(OBJDIR)/scheduler.o $(OBJDIR)/pmm.o \
          $(OBJDIR)/paging.o $(OBJDIR)/keyboard_buffer.o \
          $(OBJDIR)/terminal.o $(OBJDIR)/boot_anim.o \
          $(OBJDIR)/shell_program_blob.o

all: mykernel.iso

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(GPPPARAMS) -o $@ -c $<

$(OBJDIR)/%.o: %.s | $(OBJDIR)
	$(AS) $(ASPARAMS) -o $@ $<

$(OBJDIR)/gdt_asm.o: gdt.s | $(OBJDIR)
	$(AS) $(ASPARAMS) -o $@ $<

$(OBJDIR)/interrupts_asm.o: interrupts.s | $(OBJDIR)
	$(AS) $(ASPARAMS) -o $@ $<

shell_program.bin: src/user/shell_program.s | $(OBJDIR)
	$(AS) --32 -o $(OBJDIR)/shell_program.o $<
	$(LD) -m elf_i386 -Ttext 0x400000 --oformat binary -o $@ $(OBJDIR)/shell_program.o

$(OBJDIR)/shell_program_blob.o: shell_program.bin | $(OBJDIR)
	$(LD) -m elf_i386 -r -b binary -o $@ $<

mykernel.bin: src/arch/i386/linker.ld $(objects)
	$(LD) $(LDPARAMS) -T $< -o $@ $(objects)

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin

mykernel.iso: mykernel.bin
	mkdir -p $(ISODIR)/boot/grub
	cp $< $(ISODIR)/boot/
	echo 'set timeout=0' > $(ISODIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISODIR)/boot/grub/grub.cfg
	echo '' >> $(ISODIR)/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> $(ISODIR)/boot/grub/grub.cfg
	echo '	multiboot /boot/mykernel.bin' >> $(ISODIR)/boot/grub/grub.cfg
	echo '	boot' >> $(ISODIR)/boot/grub/grub.cfg
	echo '}' >> $(ISODIR)/boot/grub/grub.cfg
	$(MKRESCUE) -o $@ $(ISODIR)
	rm -rf $(ISODIR)

fat32.img:
	bash tools/make_fs.sh

run: mykernel.iso fat32.img
	qemu-system-i386 \
	  -boot d \
	  -cdrom mykernel.iso \
	  -drive file=fat32.img,format=raw,if=ide \
	  -device rtl8139,netdev=n0 \
	  -netdev user,id=n0

clean:
	rm -rf $(OBJDIR) $(ISODIR)
	rm -f mykernel.bin mykernel.iso shell_program.bin
	rm -f *.o *.bin *.iso
