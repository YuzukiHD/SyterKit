#!/usr/bin/env bash

set -euo pipefail

log="$1"
elf="$2"
nm_tool="$3"
objdump_tool="$4"
minstack="$5"
clean_log="$(mktemp)"
disassembly="$(mktemp)"
trap 'rm -f "$clean_log" "$disassembly"' EXIT

tr -d '\r' < "$log" > "$clean_log"
grep -Fqx 'TEST PASS arm_startup' "$clean_log"
! grep -Fq 'CHECK FAIL' "$clean_log"

symbol_value() {
	"$nm_tool" -n "$elf" | awk -v symbol="$1" \
		'$3 == symbol { print "0x" $1; exit }'
}

require_symbol() {
	local value
	value="$(symbol_value "$1")"
	[[ -n "$value" ]] || {
		echo "test: missing ELF symbol $1" >&2
		exit 1
	}
	printf '%s' "$value"
}

spl_start=$(( $(require_symbol __spl_start) ))
entry=$(( $(require_symbol _start) ))
(( entry == spl_start + 0x40 ))

for mode in und abt irq fiq; do
	start=$(( $(require_symbol "__stack_${mode}_start") ))
	end=$(( $(require_symbol "__stack_${mode}_end") ))
	if [[ "$minstack" == 1 ]]; then
		(( end == start ))
	else
		(( end - start == 0x100 ))
	fi
done

svc_start=$(( $(require_symbol __stack_srv_start) ))
svc_end=$(( $(require_symbol __stack_srv_end) ))
(( svc_end - svc_start == 0x1000 ))
(( svc_end % 16 == 0 ))

"$objdump_tool" -d "$elf" > "$disassembly"
awk '
/<reset>:/ { in_reset = 1 }
in_reset && /<main>/ { after_main = 1 }
after_main && /[[:space:]]wfi([[:space:]]|$)/ { found = 1; exit }
END { exit !found }
' "$disassembly"
