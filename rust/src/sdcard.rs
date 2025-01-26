use embedded_sdmmc::{BlockDevice, TimeSource, VolumeManager};

const MAX_LOAD_FILES: usize = 5;

pub enum SdCardError<E: core::fmt::Debug> {
    OpenVolume(embedded_sdmmc::Error<E>),
    OpenRootDir(embedded_sdmmc::Error<E>),
    CloseRootDir(embedded_sdmmc::Error<E>),
    LoadFile(heapless::Vec<embedded_sdmmc::Error<E>, MAX_LOAD_FILES>),
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

    let mut load_file_errors = heapless::Vec::new();
    for (filename, addr, size) in files {
        let ans = unsafe { load_file_into_memory(&mut volume_mgr, root_dir, filename, addr, size) };
        if let Err(e) = ans {
            // discard extra errors
            let _ = load_file_errors.push(e);
        }
    }
    if load_file_errors.len() != 0 {
        return Err(SdCardError::LoadFile(load_file_errors));
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
) -> Result<usize, embedded_sdmmc::Error<D::Error>> {
    // Find and open the file
    volume_mgr.find_directory_entry(dir, file_name)?;

    let file = volume_mgr.open_file_in_dir(dir, file_name, embedded_sdmmc::Mode::ReadOnly)?;

    // Check file size
    let file_size = volume_mgr.file_length(file)?;
    if file_size > max_size {
        return Err(embedded_sdmmc::Error::NotEnoughSpace);
    }

    // Read file content into memory
    let target = unsafe { core::slice::from_raw_parts_mut(addr as *mut u8, file_size as usize) };
    let size = volume_mgr.read(file, target)?;

    volume_mgr.close_file(file)?;

    Ok(size)
}
