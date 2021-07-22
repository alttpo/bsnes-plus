(for x in {80..424}; do
  echo -ne "PPUX_SPR_WRITE 0;1;$x;\$130\n"
  sleep 0.02
done) | socat stdio tcp:localhost:65400
