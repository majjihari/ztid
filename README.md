## Zerotier ID generator
ztid is a command line tool to generate a unique identity files for zerotier based on some hardware information
for zero-os

It uses the following information to seed the identity generation:
- Constant Seed
- Motherboard ID (if available)
- All the mac addresses of the *physical* attached devices, sorted

> This means that a change in HW will generate a new ID

## Building
```bash
cd src
make
```

## Usage
Running the `ztid` will generate `identity.public` and `identity.secret` files in the directory privided as first argument (default to '.')

## License
The C++ code is copied from [ZeroTierOne](https://github.com/zerotier/ZeroTierOne) which is licensed under GNU GPL v3

### ZeroTierOne License
The ZeroTier source code is open source and is licensed under the GNU GPL v3 (not LGPL).
If you'd like to embed it in a closed-source commercial product or appliance,
please e-mail contact@zerotier.com to discuss commercial licensing. Otherwise it can be used for free.
