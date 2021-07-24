file="test.wasm"

/usr/local/opt/llvm/bin/clang \
  --target=wasm32 \
  -O3 \
  -flto \
  -nostdlib \
  -nostartfiles \
  -Wl,--no-entry \
  -Wl,--export-all \
  -Wl,--lto-O3 \
  -Wl,-z,stack-size=$[1048576] \
  -o "${file}" \
  test.c

size=$(stat -f %z "${file}")
size0=$((($size >> 24) & 0xFF))
size1=$((($size >> 16) & 0xFF))
size2=$((($size >>  8) & 0xFF))
size3=$((($size >>  0) & 0xFF))

size_hex=$(printf "\\\\x%02x\\\\x%02x\\\\x%02x\\\\x%02x" $size0 $size1 $size2 $size3 )
echo $size_hex
