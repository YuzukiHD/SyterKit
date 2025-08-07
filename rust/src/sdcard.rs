use crate::config::Config;
use embedded_sdmmc::{BlockDevice, TimeSource, VolumeManager};

const MAX_LOAD_FILES: usize = 5;

pub enum SdCardError<E: core::fmt::Debug> {
    OpenVolume(embedded_sdmmc::Error<E>),
    OpenRootDir(embedded_sdmmc::Error<E>),
    CloseRootDir(embedded_sdmmc::Error<E>),
    ParseConfig(picotoml::Error),
    LoadFile(heapless::Vec<embedded_sdmmc::Error<E>, MAX_LOAD_FILES>),
}

pub fn load_from_sdcard<D, T>(
    block_device: D,
    time_source: T,
    config: &mut Config,
    opaque_dst: &mut [u8],
    firmware_dst: &mut [u8],
    next_stage_dst: &mut [u8],
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

    let mut config_buf = [0u8; 1024];
    let ans = load_file_into_slice(&mut volume_mgr, root_dir, "CONFIG~1.TOM", &mut config_buf);
    if let Err(e) = ans {
        let _ = load_file_errors.push(e);
    }
    *config = crate::config::parse_config(&config_buf).map_err(|e| SdCardError::ParseConfig(e))?;

    // Must load at least one firmware, or defaults to rustsbi.bin
    let firmware_path = config.firmware.as_deref().unwrap_or("rustsbi.bin");
    let ans = load_file_into_slice(&mut volume_mgr, root_dir, firmware_path, firmware_dst);
    if let Err(e) = ans {
        let _ = load_file_errors.push(e);
    }

    if let Some(opaque_path) = config.opaque.as_deref() {
        let ans = load_file_into_slice(&mut volume_mgr, root_dir, opaque_path, opaque_dst);
        if let Err(e) = ans {
            let _ = load_file_errors.push(e);
        }
    }

    if let Some(next_stage_path) = config.next_stage.path.as_deref() {
        let ans = load_file_into_slice(&mut volume_mgr, root_dir, next_stage_path, next_stage_dst);
        if let Err(e) = ans {
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
fn load_file_into_slice<D: BlockDevice, T: TimeSource>(
    volume_mgr: &mut VolumeManager<D, T>,
    dir: embedded_sdmmc::RawDirectory,
    file_name: &str,
    target: &mut [u8],
) -> Result<usize, embedded_sdmmc::Error<D::Error>> {
    // Find and open the file
    volume_mgr.find_directory_entry(dir, file_name)?;

    let file = volume_mgr.open_file_in_dir(dir, file_name, embedded_sdmmc::Mode::ReadOnly)?;

    // Check file size
    let file_size = volume_mgr.file_length(file)? as usize;
    if file_size >= target.len() {
        return Err(embedded_sdmmc::Error::NotEnoughSpace);
    }

    // Read file content into memory
    let size = volume_mgr.read(file, target)?;

    volume_mgr.close_file(file)?;

    Ok(size)
}
