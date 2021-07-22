
__attribute__((import_module("env"), import_name("ppux_reset")))
void ppux_reset();

void on_nmi() {
  ppux_reset();
}
