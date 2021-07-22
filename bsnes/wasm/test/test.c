
// import ppux_reset()
__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

// called on NMI:
void on_nmi() {
  ppux_reset();
}
