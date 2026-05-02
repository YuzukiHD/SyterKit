# 1. 生成UBIFS镜像（修正LEB和页对齐）
mkfs.ubifs -r ../../rootfs -m 2048 -e 126976 -c 1000 -o rootfs.ubifs

# 2. 生成UBI镜像（关键：子页大小设为512，显式指定VID偏移）
ubinize -o rootfs.ubi -m 2048 -p 128KiB -s 512 -O 2048 -Q 1574823869 ubinize.cfg