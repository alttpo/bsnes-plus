echo -ne 'PPUX_SPR 0;1;$80;$70;0;0;1;5;0;4;16;16;0;240;$4000\n' | socat -x stdio tcp:localhost:65400
