# UTFS, a TAR-like File System for Embedded Systems

**Store named "files" in raw EEPROM or flash with two source files, no `malloc`, and a 24-byte header.**

UTFS is a lightweight, TAR-inspired storage layer for embedded systems that keep data in flat,
sequential, byte-addressable memory: EEPROM, CPU flash pages, or external SPI/I²C flash. It
stores named blobs ("files") back-to-back with a fixed 24-byte header each, so your firmware can
read and write structured, named data without bringing in a full file system.

- **Tiny.** Two files (`utfs.c` + `utfs.h`), C99, **zero dynamic allocation**.
- **Bring-your-own storage.** Port to any medium by implementing two functions: `sys_read` / `sys_write`.
- **Wear-friendly.** Load everything once, save everything once; you decide when to physically commit.
- **Forward-compatible format.** Add files later without breaking existing layouts.
- **No vendor lock-in.** Open format, no patents, no proprietary blobs.
- **MIT licensed.**

---

## Quick start

```c
#include "utfs.h"

// Your application data, any struct you want to persist
typedef struct {
    uint32_t test_value;
} app_data_t;

app_data_t   appdata;
utfs_file_t  appfile;

void setup(void)
{
    sys_init();                 // your storage medium init

    utfs_init(false);           // init UTFS (true = verbose logging)

    // Register the blob: name, RAM pointer, size
    utfs_set(&appfile, "appdata", &appdata, sizeof(appdata));
    utfs_register(&appfile, UTFS_NOFLAGS, UTFS_NOOPT);

    // Load all registered files from the medium
    utfs_load();

    // Use the signature to detect uninitialized / corrupt storage
    if (utfs_file_signature(&appfile) != 0xABCD) {
        memset(&appdata, 0, sizeof(appdata));      // first boot: default it
        utfs_file_signature_set(&appfile, 0xABCD);
        appdata.test_value = 1234;
    }
}

void on_change(void)
{
    appdata.test_value++;
    utfs_save();                // write all registered files back to the medium
}
```

That's the whole lifecycle: **register, load, (use), save.** No `open`/`close`, no heap, no
per-byte writes to the medium.

## Try it in 30 seconds

The `gcc_linux` example is a self-contained REPL that runs the full lifecycle against a file
(`UTFS.dat`) standing in for the medium:

```bash
cd Examples/gcc_linux
make
./main.bin -v        # interactive REPL, type `help`
```

REPL commands (`load`, `save`, `flush`, `utfs`, `status`, `value N`, `exit`) exercise loading,
saving, and inspecting stored data.

## Try it on an Arduino Uno

The `Arduino1` example builds straight from the Arduino IDE and packs **two separate files**
into the Uno's 1 KB of EEPROM: a `system` file (serial and model number) and an `appdata` file
(LED blink speed). The entire EEPROM driver is the two functions UTFS needs, wired directly to
the Arduino `EEPROM` library:

```c
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length) {
    for (uint16_t x = 0; x < length; x++) EEPROM.write(address + x, ((uint8_t*)ptr)[x]);
    return length;
}
uint32_t sys_read(uint32_t address, void * ptr, uint32_t length) {
    for (uint16_t x = 0; x < length; x++) ((uint8_t*)ptr)[x] = EEPROM.read(address + x);
    return length;
}
```

Flash it, open the serial monitor at 9600 baud, and drive it live:

```
serial ABC123      set the serial number
model  WIDGET-1    set the model number
fast                bump the LED speed
save                commit both files to EEPROM
```

Now power-cycle the board. On boot, `utfs_load()` pulls both files back out of EEPROM, the LED
resumes its saved speed, and `dump` shows your serial/model still there. Two independent,
named structs persisting across reboots, sharing one 1 KB EEPROM, with no hand-rolled offsets.
Type `eeprom` to hexdump the raw bytes and see the UTFS headers laid out back-to-back.

## Porting to your hardware

The core library does **no I/O itself**. You provide two functions against your medium:

```c
// Write `length` bytes from `ptr` to `address`; return bytes written
uint32_t sys_write(uint32_t address, void * ptr, uint32_t length);

// Read `length` bytes from `address` into `ptr`; return bytes read
uint32_t sys_read (uint32_t address, void * ptr, uint32_t length);
```

That's the entire porting seam. See each example's `sys.c` for EEPROM/flash/RAM-backed
implementations, and [`doc/README_system_interface.md`](doc/README_system_interface.md) for the
full contract.

## Footprint

UTFS is designed for resource-constrained parts:

- **No heap.** UTFS allocates nothing; it holds a small static array of *pointers* to
  caller-owned `utfs_file_t` structs (`UTFS_MAX_FILES`, default 5).
- **Your RAM cost** is your own data buffers plus that pointer table, nothing hidden.
- **On-medium overhead** is a fixed **24 bytes** per file; data is packed with no padding between files.

<!-- TODO(marketing): drop in measured .text/.data/.bss numbers for a representative target
     (e.g. Cortex-M0 -Os) once benchmarked. Numbers sell to this audience. -->

---

## Repository layout

- `src/` is the canonical reference implementation (`utfs.c`, `utfs.h`).
- `Examples/` holds self-contained, buildable ports (gcc_linux, SAMD20, Arduino1, Arduino1_min).
  Each example carries its own copy of `utfs.c`/`utfs.h`.
- `doc/` holds design docs: the `sys_read`/`sys_write` contract, use cases, and design proposals.

## In-depth articles

- [UTFS, a TAR-like File System for Embedded Systems](https://clisystems.com/article-UTFS/)
- [Using UTFS in an Embedded System](https://clisystems.com/article-UTFS-examples/)

---

## Background

Microcontroller embedded systems are commonly developed as 'bare metal' solutions, without an
operating system or built-in libraries. While many embedded systems have no operating system, it
is common for them to store and retrieve data across power cycles. This lack of system facilities
leaves the design of data storage to the developer.

Existing file systems like FAT and EXT4 are designed for humans, not embedded machines. Long
filename support, directory hierarchy, permission settings, and access attributes are common in
modern file systems. Microcontrollers and embedded systems do not typically need these human
constructs, but these features are usually not easy to disable.

Data storage for microcontroller systems commonly involves EEPROMs, CPU flash pages, or external
SPI/I²C flash chips. These chips provide a flat, sequential address space to store arbitrary data.
Storage solutions are commonly based on 'raw' reads and writes to these memories: data is loaded at
start and stored as needed in a fixed format. The limitation is that the format is difficult to
change, and the system must be designed around the storage layout. Adding data after initial
release typically results in an incompatible layout, leading to data corruption, data loss, or
broken backward compatibility with legacy systems.

UTFS addresses these shortcomings in resource-constrained systems without modern file systems. It
provides a developer-friendly interface that allows for future expansion while maintaining a
minimal code and RAM footprint.

## Goals

- Read/write arbitrary data to flat storage devices
- Provide a mechanism for accessing data as 'files'
- Allow file size to change without data loss
- Provide an application-agnostic interface
- Low code and RAM footprint
- Minimize the number of reads and writes to the medium
- Unencumbered by proprietary or undocumented vendor formats
- Unencumbered by patents
- Agnostic to the underlying memory architecture

## Scope and guarantees

UTFS is a way to **structure** data on a flat medium, not a fault-tolerant or journaling
file system. It is the disciplined replacement for the one giant `struct` that bare-metal
projects write to a fixed EEPROM address, and it inherits exactly that model's write
semantics: data is laid out plainly, and a write is a write.

What this means in practice:

- **Writes are not atomic.** A power loss partway through `utfs_save()` can leave the medium
  partially updated, the same as interrupting any raw write to EEPROM or flash. UTFS does not
  make this worse; it does not pretend to make it go away.
- **`save()` rewrites the structure from the base address.** Reordering or resizing files
  moves the bytes after them. This keeps the format trivially simple and the footprint tiny.
- **Corruption is detected at load, by you.** Set a `signature` per file, check it after
  `utfs_load()`, and default or upgrade the data if it does not match. This is the idiomatic
  way to handle blank, partial, or stale storage.

### Deliberate non-goals

UTFS does **not** implement journaling, atomic commit, wear-leveling, or bad-block
management. These are real concerns, but they are policies, and they belong in a layer you
choose, not baked into the format. Because the entire medium interface is two functions
(`sys_read` / `sys_write`), you can put that layer underneath UTFS without changing a line of
it: A/B bank the whole region and switch on a valid signature, keep a CRC'd redundant copy,
commit through a scratch page, or sit UTFS on top of an EEPROM-emulation layer that is
already atomic.

The result is a format small enough to fit the parts that have no room for a full file
system, with the structure you would otherwise hand-roll, and nothing you would have to fight
to remove.

# Theory of Operation

UTFS was inspired by the structure of the TAR format, developed to store files on tape archives.

## TAR

In TAR, a file has a 512-byte header with all file information: size, filename, user, group, and
permission settings. Later versions (the PAX standard) also have an optional extended header
section.

After the header, raw file data is written in 512-byte blocks. If file data does not fill an entire
512-byte block, the block is padded up to 512 bytes.

This header+data structure is repeated for the next file in the archive, allowing an arbitrary
number of files to be stored.

## UTFS Design

Resource-constrained systems may not have the ability to load and store 512-byte blocks.
Additionally, embedded systems commonly do not need the TAR user, group, permission, and extended
settings; they waste space on the storage medium.

UTFS implements a **24-byte header** with information about the 'file': file system identifier,
feature flags, file signature, data size, and a string name for reference in the system.

After the header, data is written, up to the size specified in the header. Subsequent files are
written header+data directly after the previous file's data.

# UTFS Data Structure

```
       | Byte0  | Byte1  | Byte2  | Byte3  |

       -------------------------------------
Word 0 |    Identifier   |Version | Flags  |
       -------------------------------------
Word 1 |    Signature    |    Reserved     |
       -------------------------------------
Word 2 |                Size               |
       -------------------------------------
Word 3 |              Filename             |
Word 4 |                                   |
Word 5 |                                   |
       -------------------------------------
```
Total header size: 24 bytes

## UTFS Header Fields

| Name | Size | Index | Description |
| --- | --- | --- | --- |
| Identifier | 2 bytes | 0 | Identifier for file format, constant `0x1984` |
| Version | 1 byte | 2 | Currently 1 |
| Flags | 1 byte | 3 | Flags for features of the file |
| Signature | 2 bytes | 4 | Signature value for the file, set by application |
| Reserved | 2 bytes | 6 | |
| Size | 4 bytes | 8 | Size in bytes of the data block |
| Filename | 12 bytes | 12 | Human-readable string to associate data |

# General Information

## Endianness

The UTFS on-medium header format is defined as **little-endian**.

> **Note:** The current reference implementation serializes the header as a raw struct, so on a
> big-endian host the multi-byte fields are written big-endian. UTFS today therefore targets
> little-endian hosts.

## Naming

UTFS stands for "micro TAR File System," where the U is the SI prefix for 10⁻⁶ (micro).

The name is officially written in all uppercase: **UTFS**.

When appropriate, the U may be lowercase or replaced with mu to indicate the small nature of the
file system: uTFS and μTFS.

## License

UTFS is released under the [MIT License](LICENSE). Copyright 2025 CLI Systems.

## Credits

- Andrew Gaylo, CLI Systems, contact@clisystems.com
