package main

import "unsafe"
import "reflect"

//export ppux_draw_list_clear
func ppux_draw_list_clear()

//export ppux_draw_list_append
func ppux_draw_list_append(size uint32, cmdlist unsafe.Pointer) uint32

//export za_file_locate
func za_file_locate(i_filename *byte) int32

//export ppux_font_load_za
func ppux_font_load_za(i_fontindex int32, i_za_fh int32)

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

var loaded bool

func str(s string) *byte {
    hdr := (*reflect.StringHeader)(unsafe.Pointer(&s))
    pbyte := (*byte)(unsafe.Pointer(hdr.Data))
    return pbyte
}

//export on_nmi
func on_nmi() {
    if !loaded {
        // load PCF font from ZIP archive:
        fh := za_file_locate(str("kakwafont-12-n.pcf"))
        ppux_font_load_za(0, fh)

        loaded = true
    }

	var module uint8
	snes_bus_read(0x7E0010, unsafe.Pointer(&module), 1)

	var cmd = [...]uint16{
		3, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x1F3F,
		3, CMD_PIXEL, 12, 118,
		7, CMD_COLOR_DIRECT_RGB888, COLOR_STROKE, 0x0000, 0xFF00, COLOR_OUTLINE, 0x00FF, 0x0000,
		8, CMD_TEXT_UTF8, 0, 2,
		   7, 0, 0, 0, 0,
	}
	copy((*(*[7]uint8)(unsafe.Pointer(&cmd[len(cmd)-4])))[:], "jsd1982")

	ppux_draw_list_clear()
	ppux_draw_list_append(uint32(len(cmd)), unsafe.Pointer(&cmd))
}

func main() {
}
