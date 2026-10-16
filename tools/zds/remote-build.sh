#!/usr/bin/env bash
# Exploratory native EMOS baseline. Run in a fresh Oryx snapshot; never flashes.
# Ordinary source composition from port/mos-agondev.mk; ZDS II/Wine only.
set -euo pipefail
zds_prefix=${AGON_ZDS_PREFIX:-"$HOME/.local/share/agon-zds/wineprefix"}
zds_root=${AGON_ZDS_ROOT:-"$zds_prefix/drive_c/Zilog/ZDSII_eZ80Acclaim!_5.3.5"}
# The installed Windows path is the launcher's default; keep vendor files external.
zds_win='C:\Zilog\ZDSII_eZ80Acclaim!_5.3.5'
stdinc="$zds_win\\include\\std;$zds_win\\include\\zilog"
usrinc='.;src;src_fatfs;src_startup;src_umm_malloc'
case ${EMOS_ZDS_OPTIMIZE:-size} in
    size) optimize=(-optsize -globalopt -globaldead -intrinsics -regvar) ;;
    speed) optimize=(-optspeed) ;;
    *) printf 'EMOS_ZDS_OPTIMIZE must be size or speed\n' >&2; exit 1 ;;
esac
mkdir -p logs
sha256sum "$zds_root/bin/ez80cc.exe" "$zds_root/bin/ez80asm.exe" "$zds_root/bin/eZ80link.exe" "$HOME/.local/bin/agon-zds" > tools.sha256
for source in main.c src/*.c src_fatfs/*.c src_umm_malloc/*.c; do
    # Ordinary EMOS excludes the separate fixed-peer qualification backend.
    [[ "$source" == src/emos_parallel_fixed_backend.c ]] && continue
    unit=$(basename "$source" .c)
    [[ "$source" == "$unit.c" ]] || cp "$source" "$unit.c"
    # ZDS's driver expects a basename in its working directory. Reject every
    # failed invocation and require complete, freshly generated assembly.
    rm -f "$unit.src" "$unit.obj"
    status=0
    "$HOME/.local/bin/agon-zds" ez80cc -cpu:eZ80F92 -NOlink -NOasm \
        "${optimize[@]}" -NOreduceopt -promote -NOdebug -genprintf -define:NDEBUG -define:_EZ80 \
        -define:_EZ80F92 '-define:_EZ80ACCLAIM!' -stdinc:"$stdinc" -usrinc:"$usrinc" \
        -keepasm -keeplst "$unit.c" > "logs/$unit.cc.log" 2>&1 || status=$?
    cat "logs/$unit.cc.log"
    if [[ $status != 0 ]] || grep -Eiq 'ERROR|cannot|failed' "logs/$unit.cc.log" || [[ ! -s "$unit.src" ]] || ! grep -Eq '^[[:space:]]*END[[:space:]]*$' "$unit.src"; then
        exit 1
    fi
    "$HOME/.local/bin/agon-zds" ez80asm -list -quiet -sdiopt -NOdebug -NOigcase \
        -cpu:eZ80F92 -include:"$stdinc" "$unit.src" > "logs/$unit.asm.log" 2>&1 || { cat "logs/$unit.asm.log"; exit 1; }
done
for source in src/*.asm src_startup/*.asm; do
    unit=$(basename "$source" .asm)
    # Assemble in the original directory for relative includes; objects are
    # copied to the link directory after assembly.
    (
        cd "$(dirname "$source")"
        "$HOME/.local/bin/agon-zds" ez80asm -list -quiet -sdiopt -NOdebug -NOigcase \
            -cpu:eZ80F92 -include:"$stdinc" "$unit.asm"
    ) > "logs/$unit.asm.log" 2>&1 || { cat "logs/$unit.asm.log"; exit 1; }
    cp "$(dirname "$source")/$unit.obj" .
done
"$HOME/.local/bin/agon-zds" eZ80link @tools/zds/mos.linkcmd | tee logs/link.log
objcopy -I ihex -O binary --gap-fill 0 MOS.hex MOS.bin
sha256sum MOS.bin MOS.hex MOS.map > image.sha256
