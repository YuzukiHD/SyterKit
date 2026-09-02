// SPDX-License-Identifier: GPL-2.0+

use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::Command;

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

fn rust_toolchain_description() -> String {
    let rustc = env::var_os("RUSTC").unwrap_or_else(|| "rustc".into());
    let version = Command::new(rustc)
        .arg("--version")
        .output()
        .ok()
        .filter(|output| output.status.success())
        .map(|output| String::from_utf8_lossy(&output.stdout).trim().to_owned())
        .filter(|version| !version.is_empty())
        .unwrap_or_else(|| "rustc (unknown version)".to_owned());
    let target = env::var("TARGET").unwrap_or_else(|_| "unknown-target".to_owned());
    format!("{version} target={target}")
}

fn main() {
    println!("cargo:rerun-if-env-changed=SYTERKIT_RUST_APP_SOURCE");
    println!("cargo:rerun-if-env-changed=SYTERKIT_FFI_CONFIG");
    println!("cargo:rerun-if-env-changed=SYTERKIT_FFI_CLANG_ARGS");
    println!("cargo:rustc-check-cfg=cfg(syterkit_rust_app)");
    println!("cargo:rustc-check-cfg=cfg(syterkit_config_driver_psram)");

    if let Ok(path) = env::var("SYTERKIT_FFI_CONFIG") {
        println!("cargo:rerun-if-changed={path}");
    }
    emit_config_cfg("CONFIG_DRIVER_PSRAM", "syterkit_config_driver_psram");

    println!(
        "cargo:rustc-env=SYTERKIT_RUST_TOOLCHAIN={}",
        rust_toolchain_description()
    );

    let Some(source) = env::var_os("SYTERKIT_RUST_APP_SOURCE") else {
        return;
    };
    let source = PathBuf::from(source);
    if !source.is_file() {
        panic!(
            "SYTERKIT_RUST_APP_SOURCE is not a Rust source file: {}",
            source.display()
        );
    }

    println!("cargo:rerun-if-changed={}", source.display());
    println!("cargo:rustc-cfg=syterkit_rust_app");
    println!(
        "cargo:rustc-env=SYTERKIT_RUST_APP_SOURCE={}",
        source.display()
    );
}
