use crate::config::Mode;

#[derive(Clone, Copy)]
#[repr(C)]
pub struct DynamicInfo {
    pub magic: usize,
    pub version: usize,
    pub next_addr: usize,
    pub next_mode: usize,
    pub options: usize,
    pub boot_hart: usize,
}

const DYNAMIC_INFO_MAGIC: usize = 0x4942534f;
const DYNAMIC_INFO_VERSION_2: usize = 2;
const RISCV_MODE_MACHINE: usize = 3;
const RISCV_MODE_SUPERVISOR: usize = 1;
const RISCV_MODE_USER: usize = 0;

impl DynamicInfo {
    /// Create dynamic information with version 2, next address 0x0, user mode with boot hart ID 0.
    #[inline]
    pub const fn new() -> Self {
        DynamicInfo {
            magic: DYNAMIC_INFO_MAGIC,
            version: DYNAMIC_INFO_VERSION_2,
            next_addr: 0,
            next_mode: 0,
            options: 0,
            boot_hart: 0,
        }
    }
    #[inline]
    pub const fn with_boot_hart(mut self, boot_hart: usize) -> Self {
        self.boot_hart = boot_hart;
        self
    }
    #[inline]
    pub const fn with_next_stage(mut self, next_mode: Mode, next_addr: usize) -> Self {
        self.next_mode = next_mode as usize;
        self.next_addr = next_addr;
        self
    }
    #[inline]
    pub const fn with_machine(mut self, next_addr: usize) -> Self {
        self.next_mode = RISCV_MODE_MACHINE;
        self.next_addr = next_addr;
        self
    }
    #[inline]
    pub const fn with_supervisor(mut self, next_addr: usize) -> Self {
        self.next_mode = RISCV_MODE_SUPERVISOR;
        self.next_addr = next_addr;
        self
    }
    #[inline]
    pub const fn with_user(mut self, next_addr: usize) -> Self {
        self.next_mode = RISCV_MODE_USER;
        self.next_addr = next_addr;
        self
    }
}
