package pbar

import (
	"fmt"
	"strings"
	"unsafe"
	"syscall"
)

const (
	barBox = 3
	percent = 5
	fraction = 24
	minw = barBox + percent + fraction
)

// defined in <sys/ioctl.h>
type winsize struct {
	ts_lines, ts_cols uint16
	ts_xxx, ts_yyy uint16
}

type PBar struct {
	total uint64
	finish uint64
	termsz winsize
}

type PBarError struct {
	why string
}

func getWindowSize() winsize {
	ws := winsize{}
	syscall.Syscall(syscall.SYS_IOCTL, uintptr(1), uintptr(syscall.TIOCGWINSZ),
		uintptr(unsafe.Pointer(&ws)))
	return ws
}

func (e *PBarError) Error() string {
	return e.why
}

func (bar *PBar) updateBar() {
	if bar.termsz.ts_cols <= minw {
		// fall back to text mode
	}
	// xxx bw for bar width, I should try to make it even
	var bw = bar.finish * uint64(bar.termsz.ts_cols - minw)  / bar.total
	// really ugly
	// fmt.Print(strings.Replace(fmt.Sprintf("\r%0*d", bw, 0), "0", "=", -1))

	fmt.Print("\r" +
		// percentage number includes the '%' and a space
		fmt.Sprintf("%*d%% ", percent-2, int(bar.finish * 100 / bar.total)) +
		"[" + strings.Repeat("=", int(bw)) + ">" +
		strings.Repeat(" ", int(bar.termsz.ts_cols - minw - uint16(bw))) +"]")

	if bar.finish == bar.total {
		fmt.Print("\n")
	}
}

func (bar *PBar) settotal(target uint64) error {
	if target < bar.finish {
		return &PBarError{ "total is less than finish" }
	}
	bar.total = target
	return nil
}

func (bar *PBar) check(inc uint64) error {
	bar.finish += inc
	if bar.finish > bar.total {
		return &PBarError{ "Task already done" }
	}
	bar.updateBar()
	return nil
}

func (bar *PBar) Start(target uint64, c chan uint64) error {
	bar.total  = target
	for inc := range c {
		bar.termsz = getWindowSize()
		bar.check(inc)
	}
	return nil
}
