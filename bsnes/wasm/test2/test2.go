package main

import "unsafe"

//export ppux_draw_list_clear
func ppux_draw_list_clear()

//export ppux_draw_list_append
func ppux_draw_list_append(size uint32, cmdlist unsafe.Pointer) uint32

const (
	COLOR_STROKE = iota
	COLOR_FILL
	COLOR_OUTLINE

	COLOR_MAX
)

const color_none uint16 = 0x8000

const (
    // commands which affect state:
    ///////////////////////////////
    CMD_TARGET uint16 = iota + 1
    CMD_COLOR_DIRECT_BGR555
    CMD_COLOR_DIRECT_RGB888
    CMD_COLOR_PALETTED
    CMD_FONT_SELECT
)
const (
    // commands which use state:
    ///////////////////////////////
    CMD_TEXT_UTF8 uint16 = iota + 0x40
    CMD_PIXEL
    CMD_HLINE
    CMD_VLINE
    CMD_LINE
    CMD_RECT
    CMD_RECT_FILL
)
const (
    // commands which ignore state:
    ///////////////////////////////
    CMD_VRAM_TILE uint16 = iota + 0x80
    CMD_IMAGE
)

//export snes_bus_read
func snes_bus_read(i_address uint32, i_data unsafe.Pointer, i_size uint32)

//export on_nmi
func on_nmi() {
	var module uint8
	snes_bus_read(0x7E0010, unsafe.Pointer(&module), 1)

	var cmd [8]uint16 = [8]uint16{
		3, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x1F3F,
		3, CMD_PIXEL, 12, 118,
	}
	ppux_draw_list_clear()
	ppux_draw_list_append(uint32(len(cmd)), unsafe.Pointer(&cmd))
}

func main() {
}
