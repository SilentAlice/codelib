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

// borrowed from github.com/scorredoira/email
var boundary = "f46d043c813270fc6b04c2d223da"

func oneLiner(prompt string) string {
	reader := bufio.NewReader(os.Stdin)

	fmt.Printf("%s", prompt)
	line, err := reader.ReadString('\n')

	if err != nil {
		fmt.Fprintf(os.Stderr, "read stdin error\n")
		panic(err)
	}

	return strings.Trim(line, "\r\n")
}

// getAuthInfo get smtp authentication username and password
// from command line using gopass
func getAuthInfo() (string, string) {
	username := oneLiner("Enter username: ")

	password, passerr := gopass.GetPass("Enter password: ")
	if passerr != nil {
		fmt.Fprintf(os.Stderr, "get password error\n")
		panic(passerr)
	}

	return username, password
}

// readStudentInfo reads student email addresses and name strings
// from the contact file
func readStudentInfo() []SInfo {
	// read email addresses from a file
	addrListPath := oneLiner("Directory with address list: ")
	addrListFile, err := os.Open(addrListPath)
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

func readAttachInfo() map[string][]byte {
	attachments := make(map[string][]byte)

	for {
		filename := oneLiner("Attachment filename: ")

		if filename == "" {
			return attachments
		}

		content, err := ioutil.ReadFile(filename)
		if err != nil {
			panic(err)
		}
		attachments[filename] = content
	}
}

func readMailBody() (body []byte) {
	filename := oneLiner("Body file path: ")

	body, err := ioutil.ReadFile(filename)
	if err != nil {
		panic(err)
	}
	return body
}

// makeMessage generates message contents for each student
// Attachment part of this function is copied and modified from
// https://github.com/scorredoira/email
func makeMessage(each SInfo, header map[string]string, body []byte, attachments map[string][]byte) (msg string) {
	var buf bytes.Buffer // A Buffer needs no initialization.

	for k, v := range header {
		buf.WriteString(k)
		buf.WriteString(": ")
		buf.WriteString(v)
		buf.WriteString("\r\n")
	}

	if len(attachments) > 0 {
		buf.WriteString("Content-Type: multipart/mixed; boundary=" + boundary + "\r\n")
		buf.WriteString("--" + boundary + "\r\n")
	}

	buf.WriteString("Content-Type: text/plain; charset=\"utf-8\"\r\n")

	// start of body
	buf.Write(body)
	buf.Write([]byte("\r\n"))

	// start of attachment
	for name, content := range attachments {
		buf.WriteString("--" + boundary + "\r\n")
		buf.WriteString("Content-Type: application/octet-stream\n")
		buf.WriteString("Content-Transfer-Encoding: base64\n")
		buf.WriteString("Content-Disposition: attachment; filename=\"" + path.Base(name) + "\"\n\n")
		b := make([]byte, base64.StdEncoding.EncodedLen(len(content)))
		base64.StdEncoding.Encode(b, content)
		buf.Write(b)
		buf.WriteString("\n--" + boundary + "\r\n")
	}

	msg, _ = buf.ReadString(byte(0))
	fmt.Println(msg)
	return msg
}

func main() {
	username, password := getAuthInfo()

	sl := readStudentInfo()

	attachments := readAttachInfo()

	title := oneLiner("Title: ")

	body := readMailBody()

	fmt.Println(sl)
	// to avoid server recognition, use gmail as default
	auth := smtp.PlainAuth("", username, password, "smtp.gmail.com")

	// fill message with student lab grades
	for _, each := range sl {
		header := make(map[string]string)
		header["From"] = username
		header["To"] = each.addr
		header["Subject"] = title
		header["MIME-Version"] = "1.0"
		// header["Content-Transfer-Encoding"] = "base64"

		msgstr := []byte(makeMessage(each, header, body, attachments))

		if err := smtp.SendMail("smtp.gmail.com:587", auth, username, []string{each.addr}, msgstr); err != nil {
			fmt.Println(err)
		} else {
			fmt.Printf("send ok %s\n", each.addr)
		}
	}
}
