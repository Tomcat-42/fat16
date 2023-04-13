# SPDX-License-Identifier: GPL-2.0
ccflags-y := -I$(src)/include
NAME := fat16
obj-m := $(NAME).o

$(NAME)-y := src/main.o src/fat16/super.o src/fat16/inode.o src/fat16/dentry.o src/fat16/file.o

