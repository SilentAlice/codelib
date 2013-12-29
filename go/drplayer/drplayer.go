package main

import (
	"bufio"
	// "encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"
)

var opt struct {
	file string
}

// func unpack(template string,

func init() {
	flag.StringVar(&opt.file, "f", "", "filename of the binary log")
}

func main() {
	flag.Parse()
	if opt.file == "" {
		fmt.Println("empty file")
		return
	}
	f, err := os.Open(opt.file)
	if err != nil {
		fmt.Println("can't open file")
		return
	}

	fr := bufio.NewReader(f)
	for {
		str, err := fr.ReadString('\n')
		if err == io.EOF {
			break
		}
		if err != nil {
			fmt.Println(err)
			break
		}
		fmt.Printf("%s", str)
	}
	// binary.Read(bufio.NewReader(f), binary.LittleEndian,
}
