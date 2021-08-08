
/usr/local/opt/llvm/bin/clang \
  --target=wasm32 \
  -O3 \
  -nostdlib \
  -nostartfiles \
  -fno-builtin \
  -Wl,--no-entry \
  -Wl,-z,stack-size=$[32768] \
  -o "test.wasm" \
  test.c
