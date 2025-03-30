# Roadmap

## Branding
- [x] Figure out a name for the project.
- [ ] Design logo and visual identity.

## Boot & Initial System Setup
- [x] Build a cross-compiler toolchain.
- [x] Boot into x86_64.
- [ ] UEFI support.
- [x] GDT Initialization.
- [ ] IDT Initialization.
- [ ] SIMD support.

## Basic I/O
- [ ] Console TTY (with PS/2 keyboard input).
- [ ] Serial port driver.

## Memory Management
- [ ] Basic memory management.
  - [ ] Physical memory manager.
  - [ ] Virtual memory manager.

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
- [ ] APIC
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

## POSIX Compatibility
_TODO_
