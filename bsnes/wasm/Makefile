wasm_objects := wasm-host wasm-interface
objects += $(wasm_objects)

link += -L../external/wasm3/build/source -lm3
flags += -I../external/wasm3/source

$(objdir)/wasm-host.o: $(wasm)/host.cpp $(wasm)/host.hpp
$(objdir)/wasm-interface.o: $(wasm)/wasminterface.cpp $(wasm)/wasminterface.hpp $(wasm)/wasm_bindings.cpp $(wasm)/drawlist.cpp $(wasm)/drawlist.hpp $(wasm)/pixelfont.cpp $(wasm)/pixelfont.hpp $(wasm)/utf8decoder.cpp
