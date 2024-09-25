use std::process::Command;

fn main() {
    let output = Command::new("git").args(&["rev-parse", "HEAD"]).output();
    match output {
        Ok(output) => {
            let git_hash = String::from_utf8(output.stdout).unwrap();
            let git_hash = &git_hash[..8];
            println!("cargo:rustc-env=SYTERKIT_GIT_HASH={}", git_hash);
        }
        Err(e) => {
            panic!(
                "git command failed on host when compiling SyterKit: {}
during compilation, git must be found, as it is used to generate the git hash version of the current package.",
                e
            );
        }
    }
    let syterkit_rustc_version = {
        let version = rustc_version::version_meta().unwrap();
        let hash_date_extra = match (version.commit_hash, version.commit_date) {
            (Some(commit_hash), Some(commit_date)) => {
                format!(" ({} {})", &commit_hash[..8], commit_date)
            }
            (Some(commit_hash), None) => format!(" ({})", &commit_hash[..8]),
            (None, Some(commit_date)) => format!(" ({})", commit_date),
            (None, None) => "".to_string(),
        };
        format!("{}{}", version.semver, hash_date_extra)
    };
    println!(
        "cargo:rustc-env=SYTERKIT_RUSTC_VERSION={}",
        syterkit_rustc_version
    );
}
