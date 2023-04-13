# fat16 linux kernel driver

A simple fat16 implementation for the linux kernel.

## Running

Are you feeling lucky?

### Yes I am!!!

Change the makefile, or pass `$KDIR=/lib/modules/$(uname-r)/build` to point to your kernel headers:

```bash
make
# SPDX-License-Identifier: GPL-2.0

KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) LLVM=1 -j$(nproc) -C $(KDIR) M=$$PWD
```

and then run:

```bash
make
sudo insmod fat16.ko
# May god have mercy on your soul
```

### Maybe?

Run the provided linux vm demo with qemu locally:

```bash
qemu-system-x86_64 \
        -kernel assets/vmlinux \
        -append "console=ttyS0,115200 root=/dev/ram0 loglevel=3" \
        -initrd assets/initramfs.img \
        -nographic \
        -machine q35 \
        -enable-kvm \
        -device intel-iommu \
        -cpu host \
        -m 1G \
        -nic user,model=rtl8139,hostfwd=tcp::5555-:23,hostfwd=tcp::5556-:8080
```

### Absolutely not!

Just run the provided linux vm demo with [Docker](https://www.docker.com/):

```bash
docker run --device=/dev/kvm -it tomcat0x42/fat16
```

## Testing

After the module has been loaded, you can test it with the provided test script:

```bash
./test.sh
```

or mount the image and test it manually:

```bash
mount -o loop fat16_1sectorpercluster.img ./1sectorpercluster
mount -o loop fat16_4sectorspercluster.img ./4sectorspercluster
```




