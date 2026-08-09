# Prebuilt host tools

`dt2c` is built from the exact source revision pinned by the
`tools/dt2c` submodule. The checked-in executable targets Linux x86_64 and is
statically linked with musl, so the normal firmware build does not require a
Rust toolchain or a particular host libc.

Other hosts build dt2c from the submodule with Cargo. Set `DT2C` and, when the
headers are not adjacent to it, `DT2C_INCLUDE` to use another prebuilt release.
