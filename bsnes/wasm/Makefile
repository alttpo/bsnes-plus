wasm_objects := wasm-host wasm-interface
objects += $(wasm_objects)

link += -L../external/wasm3/build/source -lm3
flags += -I../external/wasm3/source

$(objdir)/wasm-host.o: $(wasm)/host.cpp
$(objdir)/wasm-interface.o: $(wasm)/wasminterface.cpp $(wasm)/wasm_bindings.cpp
