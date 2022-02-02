
set -e

if  [[ $OS == "Windows_NT" ]]; then
  wasmclang=clang
else
  wasmclang=/usr/local/opt/llvm/bin/clang
fi

$wasmclang \
  --target=wasm32 \
  -g \
  -nostdlib \
  -nostartfiles \
  -fno-builtin \
  -Wl,--no-entry \
  -Wl,-z,stack-size=$[32768] \
  -o "main.wasm" \
  test.c

zip test.zip main.wasm kakwafont-12-n.pcf || /c/Program\ Files/7-Zip/7z.exe u test.zip main.wasm kakwafont-12-n.pcf
