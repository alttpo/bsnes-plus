package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io/ioutil"
	"net"
	"os"
	"path/filepath"
	"time"
)

func main() {
	var err error

	fReset := flag.Bool("reset", false, "Reset WASM")
	flag.Parse()

	args := flag.Args()
	if len(args) < 1 {
		fmt.Println("send-wasm [-reset] <wasm file> <wasm file> ...")
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

	var n int
	b := bytes.Buffer{}
	if *fReset {
		b.WriteString("WASM_RESET\n")
		_, err = c.Write(b.Bytes())
		if err != nil {
			panic(err)
		}
	}

	for _, path := range args {
		var fb []byte
		fb, err = ioutil.ReadFile(path)
		if err != nil {
			panic(err)
		}

		b.Reset()
		b.Grow(len(fb) + 20)

		// strip extension off filename:
		moduleName := filepath.Base(path)
		moduleName = moduleName[0:len(moduleName)-len(filepath.Ext(moduleName))]

		_, _ = fmt.Fprintf(&b, "WASM_LOAD %s\n", moduleName)
		b.WriteByte(0)
		err = binary.Write(&b, binary.BigEndian, uint32(len(fb)))
		if err != nil {
			panic(err)
		}
		n, err = b.Write(fb)
		if err != nil {
			panic(err)
		}

		//fmt.Print(hex.Dump(b.Bytes()))
		_, err = c.Write(b.Bytes())
		if err != nil {
			panic(err)
		}

		err = c.SetReadDeadline(time.Now().Add(time.Second))
		if err != nil {
			panic(err)
		}
		rsp := make([]byte, 65536)
		n, err = c.Read(rsp)
		if err != nil {
			panic(err)
		}
		rsp = rsp[:n]

		_, _ = os.Stdout.Write(rsp)
	}
}
