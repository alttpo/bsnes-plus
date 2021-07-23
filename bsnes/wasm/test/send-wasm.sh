file="test.wasm"
size=$(stat -f %z "${file}")
size0=$((($size >> 24) & 0xFF))
size1=$((($size >> 16) & 0xFF))
size2=$((($size >>  8) & 0xFF))
size3=$((($size >>  0) & 0xFF))

size_hex=$(printf "\\\\x%02x\\\\x%02x\\\\x%02x\\\\x%02x" $size0 $size1 $size2 $size3 )
echo $size_hex

# TODO: update \x01\x95 to length of test.wasm file
(echo -ne 'WASM_RESET\nWASM_LOAD test\n\x00\x00\x00\x03\xBE'; cat test.wasm) | socat stdio tcp:127.0.0.1:65400

#echo -ne 'WASM_INVOKE on_nmi\n' | socat stdio tcp:127.0.0.1:65400
