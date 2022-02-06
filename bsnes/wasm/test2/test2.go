package main

import "encoding/binary"
import "math"
import "unsafe"

type log_level int32
const (
  L_DEBUG log_level = iota
  L_INFO
  L_WARN
  L_ERROR
)

//go:wasm-module env
//go:export log_go
func log(level log_level, msg string);

//go:wasm-module env
//go:export za_file_locate_go
func za_file_locate(i_filename string, o_fh *uint32) int32

//go:wasm-module snes
//go:export bus_read
func bus_read(i_address uint32, i_data unsafe.Pointer, i_size uint32)

//go:wasm-module snes
//go:export ppux_draw_list_clear
func ppux_draw_list_clear()

//go:wasm-module snes
//go:export ppux_draw_list_resize
func ppux_draw_list_resize(len uint32)

//go:wasm-module snes
//go:export ppux_draw_list_set
func ppux_draw_list_set(index uint32, size uint32, cmdlist unsafe.Pointer) uint32

//go:wasm-module snes
//go:export ppux_draw_list_append
func ppux_draw_list_append(size uint32, cmdlist unsafe.Pointer) uint32

//go:wasm-module snes
//go:export ppux_font_load_za
func ppux_font_load_za(i_fontindex int32, i_za_fh uint32) bool

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

var cmd = [...]uint16{
    3, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x1F3F,
    3, CMD_PIXEL, 12, 118,
    7, CMD_COLOR_DIRECT_RGB888, COLOR_STROKE, 0x00FF, 0x3F00, COLOR_OUTLINE, 0x005F, 0x1F00,
    8, CMD_TEXT_UTF8, 0, 2,
       7, 0, 0, 0, 0,
}

var f float64

//go:export on_nmi
func on_nmi() {
	var module uint8
	bus_read(0x7E0010, unsafe.Pointer(&module), 1)

    v := math.Sin(f * math.Pi)
    f += math.Pi / 256.0
    binary.LittleEndian.PutUint16(
        (*(*[2]uint8)(unsafe.Pointer(&cmd[len(cmd)-7])))[:],
        uint16(v * 127.0 + 128.0))

	ppux_draw_list_set(0, uint32(len(cmd)), unsafe.Pointer(&cmd))
}

//go:export on_power
func on_power() {
	ppux_draw_list_clear()
	ppux_draw_list_resize(1)
}

func main() {
    // load PCF font from ZIP archive:
    var fh uint32
    log(L_DEBUG, "load PCF font")
    if za_file_locate("kakwafont-12-n.pcf", &fh) == 0 {
        ppux_font_load_za(0, fh)
    }

	copy((*(*[7]uint8)(unsafe.Pointer(&cmd[len(cmd)-4])))[:], "jsd1982")
}
