## Zerotier ID generator
ztid is a command line tool to generate a unique identity files for zerotier based on some hardware information
for zero-os

It uses the following information to seed the identity generation:
- Constant Seed
- Motherboard ID (if available)
- All the mac addresses of the *physical* attached devices

> This means that a change in HW will generate a new ID

## Building
```bash
go build
```

## Usage
Running the `ztid` will generate `identity.public` and `identity.secret` files in the `-out` directory (default to '.')