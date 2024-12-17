if (CONFIG_CHIP_UFS)
set(UFS_DRIVER
    ufs/ufs.c
    ufs/ufs-phy.c
    ufs/scsi.c
)
endif()