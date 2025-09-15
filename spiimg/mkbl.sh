cp ../build/app/syter_boot_spi/syter_boot_spi_bin_spi.bin .
rm -f ./spi.img
dd if=syter_boot_spi_bin_spi.bin of=spi.img bs=2k
dd if=syter_boot_spi_bin_spi.bin of=spi.img bs=2k seek=32
dd if=syter_boot_spi_bin_spi.bin of=spi.img bs=2k seek=64