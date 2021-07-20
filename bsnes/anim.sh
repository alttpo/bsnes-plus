(for x in {80..324}; do
  echo -ne "PPUX_SPR_WRITE 0;1;$x;\$130|1;1;$x;\$138\n"
  sleep 0.05
done) | socat stdio tcp:localhost:65400
