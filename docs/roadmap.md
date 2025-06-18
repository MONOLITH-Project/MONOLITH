# Roadmap

## Branding

- [x] Figure out a name for the project.
- [ ] Design logo and visual identity.

## Boot & Initial System Setup

- [x] Build a cross-compiler toolchain.
- [x] Boot into x86_64.
- [x] UEFI support.
- [x] GDT Initialization.
- [x] IDT Initialization.

## Basic I/O

- [x] Serial port driver.
- [ ] Serial Console TTY.
- [x] Framebuffer Console TTY.
- [x] PS/2 keyboard driver.
- [ ] PS/2 Mouse driver.
- [ ] USB keyboard driver.

## Memory Management

- [x] Physical memory manager.
- [x] Virtual memory manager.
- [x] Heap allocator.

## Storage

- [x] Virtual File System.
- [ ] ISO9660 filesystem support.
- [ ] FAT filesystem support.
  - [ ] Read.
  - [ ] Write.
  - [ ] Create files.
  - [ ] Format partitions.
- [ ] EXT2 filesystem support.
  - [ ] Read.
  - [ ] Write.
  - [ ] Create files.
  - [ ] Format partitions.

## CPU & Multiprocessing

- [x] SIMD (SSE, AVX, ...).
- [x] PIT Timer.
- [ ] APIC.
- [ ] APIC timer.
- [ ] HPET.
- [ ] SMP.
- [ ] Multi-tasking.

## User Mode & Process Management

- [ ] User-space.
- [ ] ELF loader.
- [ ] System calls.
- [ ] IPC.

## Architecture support

- [x] x86_64.
- [x] ~~i386.~~ (Deprecated)
- [ ] aarch64.
- [ ] mips.
- [ ] risc-v.
- [ ] ppc.

## Hardware Support

- [ ] ACPI.
- [ ] PCI.
- [ ] USB.
- [ ] IDE/ATA disk driver.
- [ ] AHCI disk driver.

## Video and Graphics

- [ ] VESA Driver.
  - [x] i386.
  - [ ] x86_64.
- [ ] Bochs Display Adapter.
- [ ] GOP Driver.
- [ ] GUI.
- [ ] Graphics library.
- [ ] Font rendering.
- [ ] Window manager.

## Networking

- [ ] Ethernet driver.
- [ ] TCP/IP stack.
  - [ ] IPv4 implementation.
  - [ ] IPv6 implementation.
  - [ ] TCP protocol.
  - [ ] UDP protocol.
  - [ ] ICMP protocol (ping).
  - [ ] Socket API.
- [ ] DHCP client.
- [ ] DNS client.
- [ ] TLS/SSL support.

## Syscalls

- [ ] Process Management
  - [ ] fork
  - [ ] exec
  - [ ] exit
  - [ ] wait
  - [ ] getpid
- [ ] Memory Management
  - [ ] mmap
  - [ ] munmap
  - [ ] brk
- [ ] File Operations
  - [ ] open
  - [ ] close
  - [ ] read
  - [ ] write
  - [ ] lseek
  - [ ] stat
  - [ ] ioctl
  - [ ] fcntl
- [ ] Directory Operations
  - [ ] opendir
  - [ ] readdir
  - [ ] mkdir
  - [ ] rmdir
- [ ] File System
  - [ ] mount
  - [ ] umount
  - [ ] chdir
  - [ ] getcwd
