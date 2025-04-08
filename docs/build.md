# Build instructions

## Pre-requisites

### Fedora / RHEL

```console
sudo dnf install gcc gcc-c++ gcc-devel.i686 make bison flex gmp-devel libmpc-devel mpfr-devel texinfo isl-devel qemu-system-x86_64 nasm make grub
```

### Arch / Manjaro
```console
sudo pacman -S base-devel qemu-desktop flex texinfo grub nasm libmpc gmp mpfr libisl flex
```

_TODO: Add instructions for other operating systems._

## Building the operating system

```console
make run
```
