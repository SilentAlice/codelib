// Copyright 2013 Yang Hong. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Program mailthem implements a simple SMTP mail deliver tool
package main

import (
	"fmt"
	"bufio"
	"os"
	"net/smtp"
	"code.google.com/p/gopass"
)

func getAuthInfo() (string, string, error) {
	reader := bufio.NewReader(os.Stdin)

	fmt.Print("Enter username: ")
    username, _ := reader.ReadString('\n')

	password, passerr := gopass.GetPass("Enter password: ")
	return username, password, passerr
}

func main() {
	username, password, err := getAuthInfo()
	if err != nil {
		fmt.Println(err)
	}

	// XXX read email addresses from a file
	recver := []string{ "andygordo@163.com" }

	// to avoid server recognition, use gmail as default
	auth := smtp.PlainAuth("", username, password, "smtp.gmail.com")

	// XXX will fill message with student lab grades
	msg := "hello"
	msgbytes := []byte(msg)

	err = smtp.SendMail("smtp.gmail.com:587", auth, username, recver, msgbytes);
	if err != nil {
		fmt.Println(err)
	}
}







