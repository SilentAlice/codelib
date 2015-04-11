package hhash

import (
	"log"
	"strconv"
	"testing"
)

var hm Handlemap

func TestAllocString(t *testing.T) {
	hm.Init()

	for i := 0; i < 1024; i++ {
		hm.AllocString("hello" + strconv.Itoa(i))
	}
}

func TestDump(t *testing.T) {
	log.Printf("hm.tab size now %d\n", len(hm.tab))
}
