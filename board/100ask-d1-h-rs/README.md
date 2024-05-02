# 100ASK D1-H Dual Display DevKit

## Build package

Use following command on any path under SyterKit project:

```
cargo build -p syterkit-100ask-d1-h
```

Use following command to check build result ([`cargo-binutils`](https://github.com/rust-embedded/cargo-binutils) is required):

```
rust-objdump -d target\riscv64imac-unknown-none-elf\debug\syterkit-100ask-d1-h
```
