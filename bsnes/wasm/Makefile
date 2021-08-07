wasm_objects := wasm-interface
objects += $(wasm_objects)

link += -L../external/wasmer/lib -lwasmer
flags += -I../external/wasmer/include

$(objdir)/wasm-interface.o: $(wasm)/wasminterface.cpp
