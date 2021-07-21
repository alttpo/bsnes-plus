wasm_objects := wasm-host
objects += $(wasm_objects)
link += -L../external/wasm3/build/source -lm3

wasm_flags := $(flags) -std=c++17 -I../external/wasm3/source -I../external/wasm3/platforms/cpp/wasm3_cpp/include

# implicit rules
wasm_compile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(c) $(wasm_flags) $1 -c $< -o $@, \
      $(if $(filter %.cpp,$<), \
        $(cpp) $(wasm_flags) $1 -c $< -o $@ \
      ) \
    ) \
  )

$(objdir)/wasm-host.o: $(wasm)/host.cpp; $(wasm_compile)
