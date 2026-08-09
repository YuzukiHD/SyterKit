#!/usr/bin/env bash

set -euo pipefail
diff -u "$1" "$2"
