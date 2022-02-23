
build: external/wasm3/build/source/libm3.a
	$(MAKE) -C bsnes build profile=performance

external/wasm3/build/source/libm3.a:
	$(MAKE) -C external/wasm3

clean:
	$(MAKE) -C bsnes clean
