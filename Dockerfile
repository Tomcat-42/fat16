FROM ubuntu:22.04 as runner

# Install qemu
RUN apt-get update && apt-get install -y qemu-system-x86

COPY ./assets/vmlinux /vmlinux
COPY ./assets/initramfs.img /initramfs.img


# qemu-system-x86_64 \
#         -kernel linux/vmlinux \
#         -append "console=ttyS0,115200 root=/dev/ram0 loglevel=3" \
#         -initrd busybox/initramfs.img \
#         -nographic \
#         -machine q35 \
#         -enable-kvm \
#         -device intel-iommu \
#         -cpu host \
#         -m 1G \
#         -nic user,model=rtl8139,hostfwd=tcp::5555-:23,hostfwd=tcp::5556-:8080
CMD ["qemu-system-x86_64", "--enable-kvm", "--kernel", "/vmlinux", "-initrd", "/initramfs.img", "-append", "console=ttyS0,115200 root=/dev/ram0", "--nographic", "--machine", "q35", "-device", "intel-iommu", "-cpu", "host", "-m", "1G", "-nic", "user,model=rtl8139,hostfwd=tcp::5555-:23,hostfwd=tcp::5556-:8080"]
