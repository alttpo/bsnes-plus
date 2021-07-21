wasm_objects := wasm-host
objects += $(wasm_objects)
link += -L../external/wasm3/build/source -lm3

wasm_compile = $(call compile,-I../external/wasm3/source)

$(objdir)/wasm-host.o: $(wasm)/host.cpp; $(wasm_compile)
