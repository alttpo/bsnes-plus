wasm_objects := wasminterface
objects += $(wasm_objects)

link += -L../external/wasm3/build/source -lm3
flags += -I../external/wasm3/source

$(objdir)/wasminterface.o: $(wasm)/wasminterface.cpp $(wasm)/wasminterface.hpp $(wasm)/host.cpp $(wasm)/host.hpp $(wasm)/wasm_bindings.cpp $(wasm)/drawlist.cpp $(wasm)/drawlist.hpp $(wasm)/pixelfont.cpp $(wasm)/pixelfont.hpp $(wasm)/utf8decoder.cpp
