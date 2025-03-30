# Roadmap

## Branding
- [x] Figure out a name for the project.
- [ ] Design logo and visual identity.

## Boot & Initial System Setup
- [x] Build a cross-compiler toolchain.
- [x] Boot into x86_64.
- [ ] UEFI support.
- [x] GDT Initialization.
- [x] IDT Initialization.

## Basic I/O
- [ ] Console TTY (with PS/2 keyboard input).
- [ ] Serial port driver.

## Memory Management
- [ ] Basic memory management.
  - [ ] Physical memory manager.
  - [ ] Virtual memory manager.
- [ ] Heap allocator.

## Storage
- [ ] Virtual File System.
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
- [ ] SIMD (SSE, AVX, ...).
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

## Hardware Support
- [ ] ACPI.
- [ ] PCI.
- [ ] USB.
- [ ] IDE/ATA disk driver.
- [ ] AHCI disk driver.

## Video and Graphics
- [ ] VESA Video Modes.
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

## POSIX Compatibility
_TODO_
