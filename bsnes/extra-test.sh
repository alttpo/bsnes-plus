echo -ne 'PPUX_RESET\n' | socat stdio tcp:localhost:65400

# ALTTP
#echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$80;$70;0;0;0;$8000;0;$F0;1;5;0;4;16;16\n' | socat -x stdio tcp:localhost:65400
#echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$81;$63;0;0;0;$8000;0;$F0;1;5;0;4;16;16|1;1;$82;$6B;0;0;0;$8040;0;$F0;1;5;0;4;16;16\n' | socat stdio tcp:localhost:65400

# ALTTP: "DUN"
echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$81;$63;0;0;0;$e690;0;$00;1;5;0;2;16;8\n' | socat stdio tcp:localhost:65400

#echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$30;$30;0;0;0;$f810;0;$18;1;5;0;2;160;8|1;1;$30;$38;0;0;0;$f960;0;$18;1;5;0;2;160;8\n' | socat stdio tcp:localhost:65400

#mode7
#echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$130;$130;0;0;0;$f810;0;$D8;$80;5;0;2;160;8|1;1;$130;$138;0;0;0;$f960;0;$D8;$80;5;0;2;160;8\n' | socat stdio tcp:localhost:65400
#echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$81;$63;0;0;0;$8000;0;$F0;$80;5;0;4;16;16\n' | socat stdio tcp:localhost:65400

# ALTTP copy ROM sprite sheet into extra VRAM:
#(echo -ne 'PPUX_RESET\nPPUX_RAM_WRITE VRAM;1;0;$2000\n'; (echo -ne 'CORE_READ CARTROM;$80000;$2000\n' | socat stdio tcp:localhost:65400); echo -ne 'PPUX_SPR_WRITE 0;1;$40;$40;0;0;1;$0000;0;$F0;1;5;0;4;128;128\n') > .tmpcmd
#cat .tmpcmd | socat stdio tcp:localhost:65400

# Use extra VRAM, extra CGRAM
#echo -ne 'PPUX_RESET\nPPUX_RAM_WRITE CGRAM;1;$1E0;$20\n\x00\x00\x00\x00\x20\x00\x00\xff\x7f\x7e\x23\xb7\x11\x9e\x36\xa5\x14\xff\x01\x78\x10\x9d\x59\x47\x36\x68\x3b\x4a\x0a\xef\x12\xf6\x52\x71\x15\x18\x7aPPUX_RAM_WRITE VRAM;1;0;$40\n\x00\x00\x00\x00\x40\x00\x00\x01\x00\x06\x00\x0c\x04\x38\x00\x5f\x17\x9e\x1d\xbd\x38\x00\x00\x01\x00\x07\x01\x0b\x07\x3f\x07\x68\x37\xe3\x7c\xc7\x78\x00\x00\xe0\x00\x70\x60\x4c\x70\xca\xf0\x11\xe0\x61\x80\x86\x00\x00\x00\xe0\x00\x90\xe0\xbc\xc0\x3a\xc4\xf1\x0e\xe1\x1e\x86\x78PPUX_SPR_WRITE 0;1;$81;$63;0;0;1;$0000;1;$F0;1;5;0;4;16;16|1;1;$82;$6B;0;0;1;$0040;1;$F0;1;5;0;4;16;16\n' | socat stdio tcp:localhost:65400

# Use extra VRAM, internal CGRAM
#echo -ne 'PPUX_RESET\nPPUX_RAM_WRITE VRAM;1;0;$40\n\x00\x00\x00\x00\x40\x00\x00\x01\x00\x06\x00\x0c\x04\x38\x00\x5f\x17\x9e\x1d\xbd\x38\x00\x00\x01\x00\x07\x01\x0b\x07\x3f\x07\x68\x37\xe3\x7c\xc7\x78\x00\x00\xe0\x00\x70\x60\x4c\x70\xca\xf0\x11\xe0\x61\x80\x86\x00\x00\x00\xe0\x00\x90\xe0\xbc\xc0\x3a\xc4\xf1\x0e\xe1\x1e\x86\x78PPUX_SPR_WRITE 0;1;$81;$63;0;0;1;$0000;0;$F0;1;5;0;4;16;16|1;1;$82;$6B;0;0;1;$0040;0;$F0;1;5;0;4;16;16\n' | socat stdio tcp:localhost:65400

# SM
#echo -ne 'PPUX_RESET\nPPUX_SPR_WRITE 0;1;$80;$70;0;0;0;$C000;0;$C0;1;5;0;4;16;16|1;1;$90;$70;0;0;0;$C040;0;$C0;1;5;0;4;16;16|2;1;$80;$80;0;0;0;$C080;0;$C0;1;5;0;4;16;16|3;1;$90;$80;0;0;0;$C0C0;0;$C0;1;5;0;4;16;16|4;1;$80;$90;0;0;0;$C100;0;$C0;1;5;0;4;16;16|5;1;$90;$90;0;0;0;$C140;0;$C0;1;5;0;4;16;16\n' | socat stdio tcp:localhost:65400
