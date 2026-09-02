// SPDX-License-Identifier: GPL-2.0+

use std::env;
use std::fs;

fn enabled_in_config(path: &str, name: &str) -> bool {
    fs::read_to_string(path).is_ok_and(|contents| {
        contents.lines().any(|line| {
            let line = line.trim();
            line == format!("{name}=y") || line == format!("{name}=1")
        })
    })
}

fn enabled_in_clang_args(args: &str, name: &str) -> bool {
    args.split_whitespace().any(|argument| {
        argument == format!("-D{name}=1")
            || argument == format!("-D{name}=y")
            || argument == format!("-D{name}")
    })
}

fn emit_config_cfg(name: &str, cfg_name: &str) {
    let config_enabled = env::var("SYTERKIT_FFI_CONFIG")
        .ok()
        .is_some_and(|path| enabled_in_config(&path, name));
    let clang_enabled = env::var("SYTERKIT_FFI_CLANG_ARGS")
        .ok()
        .is_some_and(|args| enabled_in_clang_args(&args, name));
    if config_enabled || clang_enabled {
        println!("cargo:rustc-cfg={cfg_name}");
    }
}

fn main() {
    println!("cargo:rerun-if-env-changed=SYTERKIT_FFI_CONFIG");
    println!("cargo:rerun-if-env-changed=SYTERKIT_FFI_CLANG_ARGS");
    println!("cargo:rustc-check-cfg=cfg(syterkit_config_driver_mmc_tuning)");
    println!("cargo:rustc-check-cfg=cfg(syterkit_config_driver_gpio_v2_pow)");

    if let Ok(path) = env::var("SYTERKIT_FFI_CONFIG") {
        println!("cargo:rerun-if-changed={path}");
    }
    emit_config_cfg(
        "CONFIG_DRIVER_MMC_TUNING",
        "syterkit_config_driver_mmc_tuning",
    );
    emit_config_cfg(
        "CONFIG_DRIVER_GPIO_V2_POW",
        "syterkit_config_driver_gpio_v2_pow",
    );
}
