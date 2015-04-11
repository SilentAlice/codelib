package hhash

import "log"

var last = 0

type Handlemap struct {
	// tab map[uint64]interface{}
	tab []string
}

func (hm *Handlemap) Init() {
	hm.tab = make([]string, 4)
	log.Printf("hm.tab size %d\n", cap(hm.tab))
}

func (hm *Handlemap) AllocGeneric(v interface{}) (val int) {
	return 0
}

func (hm *Handlemap) AllocString(s string) (val int) {
	size := cap(hm.tab)
	next := (last + 1) % size

	// the closure works like a macro?
	next_wrap := func() {
		next = (next + 1) % size
	}
	next_wrap()

	for {
		// don't allow 0, use 0 to be an invalid id
		if next == 0 {
			next_wrap()
			continue
		}

		// if already wrap around, double length
		if next == last {
			newmap := make([]string, (cap(hm.tab) * 2))
			for i, j := range hm.tab {
				newmap[i] = j
			}
			hm.tab = newmap
			// update cap for correct wrapping
			size = cap(hm.tab)
			next_wrap()
			log.Printf("enlarge: %d\n", cap(hm.tab))
			continue
		}

		if hm.tab[next] != "" {
			next_wrap()
		} else {
			hm.tab[next] = s
			break
		}
	}

	// this function should always return, crash if `make` fails
	last = next
	log.Printf("alloc idx %d\n", last)
	return next
}
