wasm_objects := wasminterface miniz
objects += $(wasm_objects)

link += -L../external/wasm3/build/source -lm3
flags += -I../external/wasm3/source

$(objdir)/miniz.o: $(wasm)/miniz.c $(wasm)/miniz.h
$(objdir)/wasminterface.o: $(wasm)/wasminterface.cpp $(wildcard $(wasm)/*.cpp) $(wildcard $(wasm)/*.hpp) $(wasm)/miniz.h
