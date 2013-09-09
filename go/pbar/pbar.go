package pbar

import (
	"fmt"
)

type PBar struct {
	Total uint64
	Finish uint64
}

type PBarError struct {
	why string
}

func (e *PBarError) Error() string {
	return e.why
}

func (bar *PBar) updateBar() {
	fmt.Printf("\rprogress: %d/%d", bar.Finish, bar.Total)
	if bar.Finish == bar.Total {
		fmt.Println("")
	}
}

func (bar *PBar) SetTotal(target uint64) error {
	if target < bar.Finish {
		return &PBarError{ "Total is less than Finish" }
	}
	bar.Total = target
	return nil
}

func (bar *PBar) check(inc uint64) error {
	bar.Finish += inc
	if bar.Finish > bar.Total {
		return &PBarError{ "Task already done" }
	}
	bar.updateBar()
	return nil
}

func (bar *PBar) Start(c chan uint64) error {
	for inc := range c {
		bar.check(inc)
	}
	return nil
}
