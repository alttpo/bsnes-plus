clang \
  --target=wasm32 \
  -O3 \
  -flto \
  -nostdlib \
  -nostartfiles \
  -Wl,--no-entry \
  -Wl,--export-all \
  -Wl,--lto-O3 \
  -Wl,-z,stack-size=$[1024] \
  -o test.wasm \
  test.c
