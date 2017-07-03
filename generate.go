package main

// #include <stdlib.h>
// #include <generate.h>
import "C"

import (
	"fmt"
	"unsafe"
)

func main() {
	var x *C.char

	C.generate(&x)

	str := C.GoString(x)
	C.free(unsafe.Pointer(x))

	fmt.Println(str)
}
