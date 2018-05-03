package main

// #include <stdlib.h>
// #include <generate.h>
import "C"

import (
	"bytes"
	"crypto/sha512"
	"flag"
	"fmt"
	"github.com/vishvananda/netlink"
	"io"
	"io/ioutil"
	"os"
	"path"
	"sort"
	"strings"
	"unsafe"
)

const (
	Seed = "2f727c7f12896af73cc89c48ca0a00d366b728eb42ae8bcdd293538fddfdc05b0d0a52d2278fa092d74f3524053e44a1fae905c8ee9af42d567e789f819e7094"
)

func Generate(seed []byte) string {
	if len(seed) != 64 {
		panic("invalid seed length")
	}

	var x *C.char

	cseed := C.CBytes(seed)
	defer C.free(cseed)

	C.generate((*C.uchar)(cseed), &x)

	str := C.GoString(x)
	C.free(unsafe.Pointer(x))

	return str
}

func GetMachineIdentity() (public, secret string) {
	links, err := netlink.LinkList()
	if err != nil {
		panic(err)
	}

	sh := sha512.New()
	io.WriteString(sh, Seed)

	//add motheroard ID if available
	if data, err := ioutil.ReadFile("/sys/devices/virtual/dmi/id/board_serial"); err == nil {
		sh.Write(bytes.TrimSpace(data))
	}

	devices := []string{}

	//all physical devices.
	for _, link := range links {
		if link.Type() != "device" {
			continue
		}

		devices = append(devices, link.Attrs().HardwareAddr.String())

	}

	// don't rely on the order of nics
	// we sort the list to ensure it's the same
	// whatever the kernel does
	sort.Strings(devices)

	for _, dev := range devices {
		io.WriteString(sh, dev)
	}

	secret = Generate(sh.Sum(nil))
	parts := strings.Split(
		secret,
		":",
	)

	//parts (addr:0:public:secret)
	public = strings.Join(parts[:3], ":")

	return
}

func main() {
	var output string
	flag.StringVar(&output, "out", ".", "Output directory where to write the identity files")
	flag.Parse()

	if output == "" {
		flag.PrintDefaults()
		os.Exit(1)
	}

	public, secret := GetMachineIdentity()

	if err := os.MkdirAll(output, 0755); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create directory: %s", output)
	}

	if err := ioutil.WriteFile(
		path.Join(output, "identity.public"),
		[]byte(public),
		0644,
	); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to write public identity file: %s", err)
		os.Exit(1)
	}

	if err := ioutil.WriteFile(
		path.Join(output, "identity.secret"),
		[]byte(secret),
		0600,
	); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to write secret identity file: %s", err)
		os.Exit(1)
	}

	os.Exit(0)
}
