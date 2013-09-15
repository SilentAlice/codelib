package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"
)

type piState struct {
	cput int // cpu temperature
}

// getCPUThermal get cpu temperature by reading device file
func getCPUThermal() (thermal int) {
	thermalFile, err := os.Open("/sys/class/thermal/thermal_zone0/temp")
	if err != nil {
		panic(err)
	}

	data := make([]byte, 10)
	cnt, err := thermalFile.Read(data)
	if err != nil {
		panic(err)
	}

	thermal, err = strconv.Atoi(strings.Trim(string(data[:cnt]), "\n"))
	if err != nil {
		panic(err)
	}

	return
}

func main() {
	ps := piState{}

	ps.cput = getCPUThermal()
	fmt.Println(ps)
}
