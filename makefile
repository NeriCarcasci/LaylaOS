GPPPARAMS = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASPARAMS = --32
LDPARAMS = -melf_i386
MKRESCUE = $(shell command -v grub2-mkrescue 2>/dev/null || command -v grub-mkrescue 2>/dev/null)

objects = loader.o kernel.o gdt.o gdt_asm.o interrupts.o interrupts_asm.o \
          keyboard.o mouse.o pci.o memorymanagement.o allocator.o \
          vga.o driver.o gui.o ata.o mbr.o fat32.o \
          ethernet.o arp.o ipv4.o icmp.o udp.o tcp.o rtl8139.o \
          tss.o syscall.o process.o scheduler.o \
          pmm.o paging.o keyboard_buffer.o terminal.o \
          boot_anim.o shell_program_blob.o


%.o: %.cpp
	g++ $(GPPPARAMS) -o $@ -c $<


%.o: %.s
	as $(ASPARAMS) -o $@ $<


gdt_asm.o: gdt.s
	as $(ASPARAMS) -o $@ $<

interrupts_asm.o: interrupts.s
	as $(ASPARAMS) -o $@ $<


shell_program.bin: shell_program.s
	as --32 -o shell_program.o shell_program.s
	ld -m elf_i386 -Ttext 0x400000 --oformat binary -o shell_program.bin shell_program.o

shell_program_blob.o: shell_program.bin
	ld -m elf_i386 -r -b binary -o shell_program_blob.o shell_program.bin

mykernel.bin: linker.ld $(objects)
	ld $(LDPARAMS) -T $< -o $@ $(objects)


install: mykernel.bin
	sudo cp $< /boot/mykernel.bin




mykernel.iso: mykernel.bin
	mkdir -p iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '	multiboot /boot/mykernel.bin' >> iso/boot/grub/grub.cfg
	echo '	boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	$(MKRESCUE) -o mykernel.iso iso
	rm -rf iso
