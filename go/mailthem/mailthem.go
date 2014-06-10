// Copyright 2013 Yang Hong. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Program mailthem implements a simple SMTP mail deliver tool
package main

import (
	"bufio"
	"bytes"
	"code.google.com/p/gopass"
	"encoding/base64"
	"fmt"
	"io"
	"io/ioutil"
	"net/smtp"
	"os"
	"path"
	"strings"
)

// SInfo student information
type SInfo struct {
	snum string
	name string
	addr string
}

var boundary = "f46d043c813270fc6b04c2d223da"

// getAuthInfo get smtp authentication username and password
// from command line using gopass
func getAuthInfo() (string, string, error) {
	reader := bufio.NewReader(os.Stdin)

	fmt.Print("Enter username: ")
	username, _ := reader.ReadString('\n')

	password, passerr := gopass.GetPass("Enter password: ")
	return strings.Trim(username, "\n"), password, passerr
}

// readStudentInfo reads student email addresses and name strings
// from the contact file
func readStudentInfo() []SInfo {
	// read email addresses from a file
	fmt.Print("Directory with address list: ")
	addrListPath, err := bufio.NewReader(os.Stdin).ReadString('\n')
	addrListFile, err := os.Open(strings.Trim(addrListPath, "\n"))
	if err != nil {
		panic(err)
	}

	addrListBuf := bufio.NewReader(addrListFile)

	// 50 students for initial set
	var sl []SInfo = make([]SInfo, 0, 50)
	for {
		line, err := addrListBuf.ReadString('\n')
		if err != nil && err != io.EOF {
			panic(err)
		}
		entry := strings.Split(strings.Trim(line, "\n"), " ")

		// make sure the line indeed has 3 parts
		if len(entry) == 3 {
			sl = append(sl, SInfo{entry[0], entry[1], entry[2]})
		}
		// the last line will return io.EOF
		if err == io.EOF {
			break
		}
	}
	return sl
}

func readAttachInfo() (filename string, content []byte) {
	fmt.Print("Attachment filename: ")

	filename, err := bufio.NewReader(os.Stdin).ReadString('\n')
	if err != nil {
		panic(err)
	}

	content, err = ioutil.ReadFile(strings.Trim(filename, "\n"))
	if err != nil {
		panic(err)
	}
	return filename, content
}

func readMailBody() (body []byte) {
	fmt.Print("Body file path: ")

	filename, err := bufio.NewReader(os.Stdin).ReadString('\n')
	if err != nil {
		panic(err)
	}

	body, err = ioutil.ReadFile(strings.Trim(filename, "\n"))
	if err != nil {
		panic(err)
	}
	return body
}

// makeMessage generates message contents for each student
func makeMessage(each SInfo, header map[string]string, body []byte, attachName string, content []byte) (msg string) {
	var buf bytes.Buffer // A Buffer needs no initialization.

	for k, v := range header {
		buf.WriteString(k)
		buf.WriteString(": ")
		buf.WriteString(v)
		buf.WriteString("\r\n")
	}

	if attachName != "" {
		buf.WriteString("Content-Type: multipart/mixed; boundary=" + boundary + "\r\n")
		buf.WriteString("--" + boundary + "\r\n")
	}

	buf.WriteString("Content-Type: text/plain; charset=\"utf-8\"\r\n")

	// start of body
	buf.Write(body)
	buf.Write([]byte("\r\n"))
	// buf.Write([]byte("Dear all,\n\r"))
	// buf.Write([]byte("  This is Distributed Computing course website and password required when downloading materials.\n\r"))
	// buf.Write([]byte("  http://zhiyuan.sjtu.edu.cn/courses/476 password：Lorenzo\n\r"))

	// buf.WriteString("\n\r\n\r----------\n\r")
	// buf.WriteString("Heng\n\r")

	// start of attachment
	buf.WriteString("--" + boundary + "\r\n")
	buf.WriteString("Content-Type: application/octet-stream\n")
	buf.WriteString("Content-Transfer-Encoding: base64\n")
	buf.WriteString("Content-Disposition: attachment; filename=\"" + path.Base(strings.Trim(attachName, "\n")) + "\"\n\n")
	b := make([]byte, base64.StdEncoding.EncodedLen(len(content)))
	base64.StdEncoding.Encode(b, content)
	buf.Write(b)
	buf.WriteString("\n--" + boundary + "\r\n")

	msg, _ = buf.ReadString(byte(0))
	fmt.Println(msg)
	return msg
}

func main() {
	username, password, err := getAuthInfo()
	if err != nil {
		fmt.Println(err)
	}

	sl := readStudentInfo()

	attach, attachContent := readAttachInfo()

	body := readMailBody()

	fmt.Println(sl)
	// to avoid server recognition, use gmail as default
	auth := smtp.PlainAuth("", username, password, "smtp.gmail.com")

	// fill message with student lab grades
	for _, each := range sl {
		title := "DC course website"
		header := make(map[string]string)
		header["From"] = username
		header["To"] = each.addr
		header["Subject"] = title
		header["MIME-Version"] = "1.0"
		// header["Content-Transfer-Encoding"] = "base64"

		msgstr := []byte(makeMessage(each, header, body, attach, attachContent))

		err = smtp.SendMail("smtp.gmail.com:587", auth, username, []string{each.addr}, msgstr)
		if err != nil {
			fmt.Println(err)
		} else {
			fmt.Printf("send ok %s\n", each.addr)
		}
	}
}
