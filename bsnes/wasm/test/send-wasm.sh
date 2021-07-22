echo -ne 'WASM_RESET\nPPUX_RESET\n' | socat stdio tcp:127.0.0.1:65400
# TODO: update \x01\x95 to length of test.wasm file
(echo -ne 'WASM_ADD\n\x00\x00\x00\x01\x95'; cat test.wasm) | socat stdio tcp:127.0.0.1:65400
#echo -ne 'WASM_INVOKE on_nmi\n' | socat stdio tcp:127.0.0.1:65400
