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

	fData := flag.String("data", "", "Send binary data with command from file")
	flag.Parse()

	args := flag.Args()
	if len(args) < 1 {
		fmt.Println("emunwcmd [-data <file>] <command> [args...]")
		return
	}

	cmd := args[0]
	if len(args) > 1 {
		args = args[1:]
	} else {
		args = args[:0]
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
	b.Grow(len(cmd) + 1)
	b.WriteString(cmd)
	if len(args) > 0 {
		argStr := strings.Join(args, ";")
		b.Grow(len(argStr) + 1)
		_, _ = fmt.Fprintf(&b, " %s", argStr)
	}
	b.WriteByte('\n')

	path := *fData
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
		n, err = b.Write(fb)
		if err != nil {
			panic(err)
		}
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
