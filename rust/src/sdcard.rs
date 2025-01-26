use embedded_sdmmc::{BlockDevice, TimeSource, VolumeManager};

pub enum SdCardError<E: core::fmt::Debug> {
    OpenVolume(embedded_sdmmc::Error<E>),
    OpenRootDir(embedded_sdmmc::Error<E>),
    CloseRootDir(embedded_sdmmc::Error<E>),
    LoadFile(),
}

pub unsafe fn load_from_sdcard<D, T>(
    block_device: D,
    time_source: T,
    files: impl IntoIterator<Item = (&'static str, usize, u32)>,
) -> Result<(), SdCardError<D::Error>>
where
    D: BlockDevice,
    T: TimeSource,
{
    let mut volume_mgr = VolumeManager::new(block_device, time_source);
    let volume0 = volume_mgr
        .open_raw_volume(embedded_sdmmc::VolumeIdx(0))
        .map_err(SdCardError::OpenVolume)?;
    let root_dir = volume_mgr
        .open_root_dir(volume0)
        .map_err(SdCardError::OpenRootDir)?;

    for (filename, addr, size) in files {
        unsafe { load_file_into_memory(&mut volume_mgr, root_dir, filename, addr, size) }?;
    }

    volume_mgr
        .close_dir(root_dir)
        .map_err(SdCardError::CloseRootDir)?;

    Ok(())
}

/// Loads a file from SD card into specified memory address
unsafe fn load_file_into_memory<D: BlockDevice, T: TimeSource>(
    volume_mgr: &mut VolumeManager<D, T>,
    dir: embedded_sdmmc::RawDirectory,
    file_name: &str,
    addr: usize,
    max_size: u32,
) -> Result<usize, SdCardError<D::Error>> {
    // Find and open the file
    volume_mgr
        .find_directory_entry(dir, file_name)
        .map_err(|_e| SdCardError::LoadFile())?;

    let file = volume_mgr
        .open_file_in_dir(dir, file_name, embedded_sdmmc::Mode::ReadOnly)
        .map_err(|_e| SdCardError::LoadFile())?;

    // Check file size
    let file_size = volume_mgr
        .file_length(file)
        .map_err(|_e| SdCardError::LoadFile())?;
    if file_size > max_size {
        return Err(SdCardError::LoadFile());
    }

    // Read file content into memory
    let target = unsafe { core::slice::from_raw_parts_mut(addr as *mut u8, file_size as usize) };
    let size = volume_mgr
        .read(file, target)
        .map_err(|_e| SdCardError::LoadFile())?;

    volume_mgr
        .close_file(file)
        .map_err(|_e| SdCardError::LoadFile())?;

    Ok(size)
}
