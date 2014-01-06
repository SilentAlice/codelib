package pack

import (
	"reflect"
)

// type of each expr element
type ExprElem struct {
	elemType interface{}
	length   uint32
}

// slice of all the element needed to parse
var elemList []ExprElem

func init() {
	// make reservation for element list
	elemList = make([]ExprElem, 10)
}

func Unpack(template string, expr string) {
}
