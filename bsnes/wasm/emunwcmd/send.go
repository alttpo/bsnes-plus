package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io/ioutil"
	"net"
	"os"
	"strings"
	"time"
)

func main() {
	var err error

	async := flag.Bool("async", false, "send all commands and then await all responses afterwards")
	flag.Parse()

	args := flag.Args()
	if len(args) < 1 {
		fmt.Println("emunwcmd <command>[;args;args...][@file] [<command>[;args;args...][@file]...]")
		return
	}

	var c *net.TCPConn
	var addr *net.TCPAddr
	addr, err = net.ResolveTCPAddr("tcp", "127.0.0.1:65400")
	if err != nil {
		panic(err)
	}

	c, err = net.DialTCP("tcp", nil, addr)
	if err != nil {
		panic(err)
	}

	for _, combo := range args {
		path := ""
		at := strings.LastIndex(combo, "@")
		if at >= 0 {
			path = combo[at+1:]
			combo = combo[0:at]
		}

		cmdargs := strings.Split(combo, ";")
		cmd := cmdargs[0]
		args := cmdargs[1:]

		b := bytes.Buffer{}
		b.Grow(len(cmd) + 1)
		b.WriteString(cmd)
		if len(args) > 0 {
			argStr := strings.Join(args, ";")
			b.Grow(len(argStr) + 1)
			_, _ = fmt.Fprintf(&b, " %s", argStr)
		}
		b.WriteByte('\n')

		if path != "" {
			var fb []byte
			fb, err = ioutil.ReadFile(path)
			if err != nil {
				panic(err)
			}

			b.Grow(1 + 4 + len(fb))

			b.WriteByte(0)
			err = binary.Write(&b, binary.BigEndian, uint32(len(fb)))
			if err != nil {
				panic(err)
			}

			_, err = b.Write(fb)
			if err != nil {
				panic(err)
			}
		}

		//fmt.Print(hex.Dump(b.Bytes()))
		_, err = c.Write(b.Bytes())
		if err != nil {
			panic(err)
		}

		if !*async {
			err = c.SetReadDeadline(time.Now().Add(time.Second))
			if err != nil {
				panic(err)
			}
			rsp := make([]byte, 65536)

			var n int
			n, err = c.Read(rsp)
			if err != nil {
				panic(err)
			}
			rsp = rsp[:n]

			_, _ = os.Stdout.Write(rsp)
		}
	}

	if *async {
		err = c.SetReadDeadline(time.Now().Add(time.Second))
		if err != nil {
			panic(err)
		}
		rsp := make([]byte, 65536)

		var n int
		n, err = c.Read(rsp)
		if err != nil {
			panic(err)
		}
		rsp = rsp[:n]

		_, _ = os.Stdout.Write(rsp)
	}
}
