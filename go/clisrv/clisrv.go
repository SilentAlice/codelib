package main

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"os"
)

// main actually there is no need to use channel
func main() {
	ch := make(chan int, 1)
	switch os.Args[1] {
	case "client":
		go func() {
			conn, err := net.Dial("tcp", "127.0.0.1:8777")
			if err != nil {
				fmt.Println("fail to connect to server")
				ch <- 1
				return
			}
			fmt.Fprintf(conn, "GET / HTTP/1.1\r\n\r\n")
			status, err := bufio.NewReader(conn).ReadString('\n')
			if err != nil {
				fmt.Println("fail to get response from server")
				ch <- 1
				return
			}
			fmt.Println(status)
			ch <- 1
		}()
		<-ch
		// do client
	case "server":
		// do server
		go func() {
			ln, err := net.Listen("tcp", ":8777")
			if err != nil {
				fmt.Println("fail to listen on port")
				ch <- 1
				return
			}
			fmt.Println("Listen on :8777")
			conn, err := ln.Accept()
			if err != nil {
				fmt.Println("Accept error")
				ch <- 1
				return
			}
			fmt.Println("received: ")
			io.Copy(conn, conn)
			conn.Close()
			ch <- 1
		}()
		<-ch
	default:
		// wrong argument
		fmt.Println("wrong argument")
	}
}
