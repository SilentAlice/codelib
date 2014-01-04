// Copyright 2013 Yang Hong. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Program mailthem implements a simple SMTP mail deliver tool
package main

import (
	"bufio"
	"bytes"
	"code.google.com/p/gopass"
	"fmt"
	"io"
	"net/smtp"
	"os"
	"strings"
)

// SInfo student information
type SInfo struct {
	snum string
	name string
	addr string
}

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

// makeMessage generates message contents for each student
func makeMessage(each SInfo, header map[string]string) (msg string) {
	var buf bytes.Buffer // A Buffer needs no initialization.

	for k, v := range header {
		buf.WriteString(k)
		buf.WriteString(": ")
		buf.WriteString(v)
		buf.WriteString("\r\n")
	}
	buf.Write([]byte("不好意思，刚才发邮件的程序写错了=。=，以下为正确内容。。。\n\n"))
	buf.Write([]byte(each.name))
	buf.Write([]byte(" 你好：\n"))
	buf.Write([]byte("Lab 2+3 的答辩安排在如下几个时间段： \n"))
	buf.Write([]byte("12 月 14 日周六上午 9 点到下午 5 点；12 月 15 日周日上午 9 点到下午 5 点；12 月 16 日周一下午 2 点到晚上 8 点"))
	buf.Write([]byte("地点在软件大楼 3402，找助教洪扬、施佳鑫、陈庆澍。答辩的内容主要是 lab2 和 lab3 的涉及到的问题，各问 1~2 个，以及关于你的代码设计问一两个问题。考虑到接近期末了，如果你已经完成 lab4 也可以一起答辩掉，可以节省你们的时间。因为时间有点久，如果你们对代码有点忘记了，建议你们可以先复习一下以前的代码和设计，lab 的一部分内容也会出现在期末试卷中，所以请认真复习。\n"))
	buf.Write([]byte("如果周六和周日你还没有完成 lab4 的话，在 12 月 20 日下午 2 点到晚上 8 店进行 lab4 的答辩\n"))
	buf.Write([]byte("如果有同学邮箱登记错误没有收到邮件，请相互告知一下。如有疑问请回复邮件，谢谢。\n"))
	buf.Write([]byte("如果你有由于时间关系有特殊要求，可以单独联系我，你可以单独来答辩，最晚最好不要超过 12 月 22 日"))

	buf.WriteString("\n\n----------\n")
	buf.WriteString("Yang Hong\n")
	buf.WriteString("hy.styx@gmail.com\n")
	buf.WriteString("Institute of Parallel and Distributed Systems")
	// msg = each.name + " 你好：\n" +

	msg, _ = buf.ReadString(byte(0))
	return msg
}

func main() {
	username, password, err := getAuthInfo()
	if err != nil {
		fmt.Println(err)
	}

	sl := readStudentInfo()
	fmt.Println(sl)
	// to avoid server recognition, use gmail as default
	auth := smtp.PlainAuth("", username, password, "smtp.gmail.com")

	// fill message with student lab grades
	for _, each := range sl {
		title := "Lab 2/3/4 答辩安排"
		header := make(map[string]string)
		header["From"] = username
		header["To"] = each.addr
		header["Subject"] = title
		header["MIME-Version"] = "1.0"
		header["Content-Type"] = "text/plain; charset=\"utf-8\""
		// header["Content-Transfer-Encoding"] = "base64"

		msgstr := []byte(makeMessage(each, header))

		err = smtp.SendMail("smtp.gmail.com:587", auth, username, []string{each.addr}, msgstr)
		if err != nil {
			fmt.Println(err)
		} else {
			fmt.Printf("send ok %s\n", each.addr)
		}
	}
}
