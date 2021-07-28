package main

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"io/ioutil"
	"net"
	"os"
	"time"
)

func main() {
	var err error
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

	var fb []byte
	fb, err = ioutil.ReadFile("test.wasm")
	if err != nil {
		panic(err)
	}

	b := bytes.Buffer{}
	b.Grow(len(fb) + 20)
	b.WriteString("WASM_RESET\nWASM_LOAD test\n")
	b.WriteByte(0)
	err = binary.Write(&b, binary.BigEndian, uint32(len(fb)))
	if err != nil {
		panic(err)
	}
	var n int
	n, err = b.Write(fb)
	if err != nil {
		panic(err)
	}

	fmt.Print(hex.Dump(b.Bytes()))
	_, err = c.Write(b.Bytes())
	if err != nil {
		panic(err)
	}

	b.Reset()
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
