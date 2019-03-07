package main

// #include <stdlib.h>
// #include <generate.h>
import "C"

import (
	"bytes"
	"crypto/sha512"
	"flag"
	"io"
	"io/ioutil"
	"os"
	"path"
	"sort"
	"strings"
	"unsafe"

	log "github.com/sirupsen/logrus"
	"github.com/vishvananda/netlink"
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
	log.Debug("calling the underlaying C generate function")
	C.generate((*C.uchar)(cseed), &x)

	str := C.GoString(x)
	C.free(unsafe.Pointer(x))

	return str
}

func GetMachineIdentity() (public, secret string) {
	log.Debug("calculating machine identity")
	links, err := netlink.LinkList()
	if err != nil {
		panic(err)
	}

	log.WithFields(log.Fields{
		"count": len(links),
	}).Debug("nics found")

	sh := sha512.New()
	io.WriteString(sh, Seed)

	//add motheroard ID if available
	log.Debugf("reading motherboard id")
	if data, err := ioutil.ReadFile("/sys/devices/virtual/dmi/id/board_serial"); err == nil {
		sh.Write(bytes.TrimSpace(data))
	} else {
		log.WithError(err).Debug("could not read motherboard id, skipping ...")
	}

	devices := []string{}

	//all physical devices.
	for _, link := range links {
		if link.Type() != "device" {
			continue
		}
		attrs := link.Attrs()
		if attrs.Name == "lo" {
			continue
		}

		log.WithFields(log.Fields{
			"nic": attrs.Name,
			"mac": attrs.HardwareAddr.String(),
		}).Debug("adding device to list of identity devices")

		devices = append(devices, attrs.HardwareAddr.String())
	}

	// don't rely on the order of nics
	// we sort the list to ensure it's the same
	// whatever the kernel does
	sort.Strings(devices)

	log.WithFields(log.Fields{
		"macs":  devices,
		"count": len(devices),
	}).Debug("macs used in identity generation (sorted)")

	for _, dev := range devices {
		io.WriteString(sh, dev)
	}

	log.Info("generating identity ...")
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
	var debug bool
	flag.StringVar(&output, "out", ".", "Output directory where to write the identity files")
	flag.BoolVar(&debug, "debug", false, "enable debug output")
	flag.Parse()

	if output == "" {
		flag.PrintDefaults()
		os.Exit(1)
	}

	if debug {
		log.SetLevel(log.DebugLevel)
	}

	public, secret := GetMachineIdentity()

	if err := os.MkdirAll(output, 0755); err != nil {
		log.WithError(err).Fatalf("failed to create directory: %s", output)
	}

	if err := ioutil.WriteFile(
		path.Join(output, "identity.public"),
		[]byte(public),
		0644,
	); err != nil {
		log.WithError(err).Fatalf("failed to write public identity file: %s", err)
	}

	if err := ioutil.WriteFile(
		path.Join(output, "identity.secret"),
		[]byte(secret),
		0600,
	); err != nil {
		log.WithError(err).Fatalf("failed to write secret identity file: %s", err)
		os.Exit(1)
	}

	os.Exit(0)
}
