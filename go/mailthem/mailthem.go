// Copyright 2013 Yang Hong. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Program mailthem implements a simple SMTP mail deliver tool
package main

import (
	"fmt"
	"bufio"
	"strings"
	"io"
	"os"
	"net/smtp"
	"code.google.com/p/gopass"
)

type SInfo struct {
	snum string
	name string
	addr string
}

func getAuthInfo() (string, string, error) {
	reader := bufio.NewReader(os.Stdin)

	fmt.Print("Enter username: ")
    username, _ := reader.ReadString('\n')

	password, passerr := gopass.GetPass("Enter password: ")
	return strings.Trim(username, "\n"), password, passerr
}

func makeMessage(each SInfo) string {
	return each.snum
}

func main() {
	username, password, err := getAuthInfo()
	if err != nil {
		fmt.Println(err)
	}

	// read email addresses from a file
	fmt.Print("Directory with address list: ")
	addrListPath, err := bufio.NewReader(os.Stdin).ReadString('\n')
	addrListFile, err := os.Open(strings.Trim(addrListPath, "\n"))
	if err != nil { panic(err) }
	addrListBuf := bufio.NewReader(addrListFile)

	// var m map[string]SInfo = make(map[string]string)
	// 50 students for initial set
	var studentList []SInfo = make([]SInfo, 0, 50)
	for {
		line, err := addrListBuf.ReadString('\n')
		if err != nil && err != io.EOF { panic(err) }
		entry := strings.Split(strings.Trim(line, "\n"), " ")

		// make sure the line indeed has 3 parts
		if len(entry) == 3 {
			studentList = append(studentList, SInfo{ entry[0], entry[1], entry[2] })
		}
		// the last line will return io.EOF
		if err == io.EOF { break }
	}

	// to avoid server recognition, use gmail as default
	auth := smtp.PlainAuth("", username, password, "smtp.gmail.com")

	// fill message with student lab grades
	for _, each := range studentList {
		msgbytes := []byte(makeMessage(each))

		err = smtp.SendMail("smtp.gmail.com:587", auth, username, []string{ each.addr }, msgbytes);
		if err != nil {
			fmt.Println(err)
		}
	}
}















