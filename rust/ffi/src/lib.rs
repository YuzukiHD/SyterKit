// SPDX-License-Identifier: GPL-2.0+

#![no_std]

/// Raw declarations generated from the selected SyterKit C header manifest.
pub mod raw {
    #![allow(
        non_camel_case_types,
        non_upper_case_globals,
        non_snake_case,
        dead_code,
        improper_ctypes,
        unsafe_op_in_unsafe_fn
    )]

    include!(env!("SYTERKIT_FFI_BINDINGS"));
}

/// Compatibility alias for consumers that need direct bindgen declarations.
pub use raw as bindings;
