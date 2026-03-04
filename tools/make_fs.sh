#!/usr/bin/env bash
set -euo pipefail

dd if=/dev/zero of=fat32.img bs=1M count=32
mkfs.fat -F 32 -n "FRODO_OS" fat32.img

mmd -i fat32.img ::/bin
mmd -i fat32.img ::/etc
mmd -i fat32.img ::/home
mmd -i fat32.img ::/home/root
mmd -i fat32.img ::/tmp
mmd -i fat32.img ::/var
mmd -i fat32.img ::/var/log

tmpdir="$(mktemp -d /tmp/frodo_fs.XXXXXX)"
cleanup() {
    rm -rf "$tmpdir"
}
trap cleanup EXIT

printf "frodo\n" > "$tmpdir/hostname"
cat > "$tmpdir/version" <<'TXT'
Frodo OS 1.0
Built for the One Ring.
TXT
cat > "$tmpdir/motd" <<'TXT'
Welcome to Frodo OS.
Keep your tools close and your backups closer.
The road goes ever on and on.
TXT
printf "Welcome, Ring-bearer. Your home directory.\n" > "$tmpdir/readme"
cat > "$tmpdir/boot.log" <<'TXT'
Frodo OS boot log
Kernel loaded OK
FAT32 mounted OK
TXT
: > "$tmpdir/.keep"

mcopy -i fat32.img "$tmpdir/hostname" ::/etc/hostname
mcopy -i fat32.img "$tmpdir/version" ::/etc/version
mcopy -i fat32.img "$tmpdir/motd" ::/etc/motd
mcopy -i fat32.img "$tmpdir/readme" ::/home/root/readme
mcopy -i fat32.img "$tmpdir/boot.log" ::/var/log/boot.log
mcopy -i fat32.img "$tmpdir/.keep" ::/tmp/.keep

echo "fat32.img created and populated."
