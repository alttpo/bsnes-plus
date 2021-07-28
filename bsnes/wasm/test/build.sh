file="test.wasm"

/usr/local/opt/llvm/bin/clang \
  --target=wasm32 \
  -O3 \
  -nostdlib \
  -nostartfiles \
  -fno-builtin \
  -Wl,--no-entry \
  -Wl,--export-all \
  -Wl,-z,stack-size=$[1048576] \
  -o "${file}" \
  test.c
