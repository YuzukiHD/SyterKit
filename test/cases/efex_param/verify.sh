#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0+

set -euo pipefail

case_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
tool="${case_dir}/../../../tools/efex-param.py"

diff -u "$1" "$2"

# main.c checks data/keys.txt against include/efex.h; check the host tool too.
python3 -B - "${tool}" "${case_dir}/data/keys.txt" <<'PY'
import importlib.util, sys
spec = importlib.util.spec_from_file_location("efex_param", sys.argv[1])
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)
for line in open(sys.argv[2]):
    name, key = line.split()
    if tool.parse_key(name) != int(key, 16) or tool.key_name(int(key, 16)) != name:
        sys.exit(f"tools/efex-param.py disagrees on {name}")
PY
