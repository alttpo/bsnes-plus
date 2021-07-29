package main

import "unsafe"

//export ppux_reset
func ppux_reset()

//export snes_bus_read
func snes_bus_read(i_address uint32, i_data unsafe.Pointer, i_size uint32);

//export on_nmi
func on_nmi() {
	ppux_reset()

	var module uint8
	snes_bus_read(0x7E0010, unsafe.Pointer(&module), 1)
}
