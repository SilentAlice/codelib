package pbar

import (
	"testing"
	"time"
)

func TestStart(t *testing.T) {
	bar := PBar{ 10, 0 }
	c := make(chan uint64)
	go bar.Start(c)
	for i := 0; i < 11; i++ {
		time.Sleep(1 * time.Second)
		c <- 1
		// err := bar.Check()
		// if err != nil {
		// 	panic(err)
		// }
	}
	close(c)
}
