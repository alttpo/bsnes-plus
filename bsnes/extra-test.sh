# ALTTP
#echo -ne 'PPUX_RESET\nPPUX_SPR_W 0;1;0;$80;$70;0;0;$8000;$F0;1;5;0;4;16;16\n' | socat -x stdio tcp:localhost:65400
echo -ne 'PPUX_RESET\nPPUX_SPR_W 0;1;0;$80;$70;0;0;$8000;$F0;1;5;0;4;16;16\n' | socat stdio tcp:localhost:65400

# SM
#echo -ne 'PPUX_RESET\nPPUX_SPR_W 0;1;0;$80;$70;0;0;$C000;$C0;1;5;0;4;16;16; 1;1;0;$90;$70;0;0;$C040;$C0;1;5;0;4;16;16; 2;1;0;$80;$80;0;0;$C080;$C0;1;5;0;4;16;16; 3;1;0;$90;$80;0;0;$C0C0;$C0;1;5;0;4;16;16; 4;1;0;$80;$90;0;0;$C100;$C0;1;5;0;4;16;16; 5;1;0;$90;$90;0;0;$C140;$C0;1;5;0;4;16;16\n' | socat stdio tcp:localhost:65400