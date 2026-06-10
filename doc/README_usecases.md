# UTFS Use Cases

## Typical use case

The typical use case for UTFS is a system that wants to collect data into several different data
structures, treat each as a 'file', and load and save them all at the same time.

Sequence of operation:

- System boots
- Call `utfs_init()` to initialize the driver
- Each code module registers its UTFS file with the driver
- Bring up the hardware so the non-volatile medium is ready to access
- Call `utfs_load()` to load all data
- Check each file's signature and default any that are blank or invalid
- All data is loaded and ready to use

A complete, compiling version of the pattern below lives in `Examples/gcc_linux` (and the
Arduino ports under `Examples/Arduino1`).

## Example

```c
#include <string.h>
#include "utfs.h"

// Signature values chosen by the application to validate each file
#define SIGNATURE_SYS   0xA1
#define SIGNATURE_APP   0xF2

// Data structures persisted as UTFS files
struct system_data {
    char serialnumber[12];
    char modelnumber[12];
};

struct application_data {
    uint32_t last_updated_timestamp;
    uint8_t  current_state;
    uint8_t  error_count;
    uint16_t app_timeout_limit;
};

// One instance of each struct, plus a utfs_file_t to describe it
struct system_data      sysdata;
struct application_data appdata;

utfs_file_t sysfile;
utfs_file_t appfile;

int main(void)
{
    utfs_init(false);            // false = quiet, true = verbose logging

    // Register each file: name, RAM pointer, size
    utfs_set(&sysfile, "system",  &sysdata, sizeof(sysdata));
    utfs_register(&sysfile, UTFS_NOFLAGS, UTFS_NOOPT);

    utfs_set(&appfile, "appdata", &appdata, sizeof(appdata));
    utfs_register(&appfile, UTFS_NOFLAGS, UTFS_NOOPT);

    system_hardware_setup();     // bring up the storage medium

    // Load every registered file from the medium
    utfs_load();

    // Validate each file by its signature. A mismatch means blank or
    // stale storage, so default the data and set the expected signature.
    if (utfs_file_signature(&sysfile) != SIGNATURE_SYS) {
        memset(&sysdata, 0, sizeof(sysdata));
        utfs_file_signature_set(&sysfile, SIGNATURE_SYS);
    }
    if (utfs_file_signature(&appfile) != SIGNATURE_APP) {
        memset(&appdata, 0, sizeof(appdata));
        utfs_file_signature_set(&appfile, SIGNATURE_APP);
    }

    // Data is now loaded and valid; the application can use it.
    while (1) {
        // ... application logic ...

        // When a value changes and should persist, update the struct
        // and save the entire file list back to the medium:
        //
        //   snprintf(sysdata.serialnumber, sizeof(sysdata.serialnumber), "%s", payload);
        //   utfs_save();
    }

    return 0;
}
```

## First boot and invalid data

On a brand-new device the medium is blank, so the first `utfs_load()` finds no valid UTFS
header and returns `RES_INVALID_FS`. The same situation can arise after a firmware change that
alters a structure's layout, where the stored bytes no longer match what the application expects.

The idiomatic way to handle both cases is the `signature` field, as shown above: after
`utfs_load()`, compare each file's signature against an application-chosen value. If it does not
match, the data is blank or stale, so default the structure and set the expected signature. The
next `utfs_save()` writes the corrected data back to the medium.

This keeps first-boot handling, corruption recovery, and structure upgrades in one simple check,
without needing UTFS to track versions for you.

## Experimental: explicit load and save flags

Flags are an experimental feature of the UTFS interface, enabled at compile time with
`UTFS_ENABLE_FLAGS`. They provide flexibility for integrating with existing systems that do not
follow the typical "load all, save all" UTFS paradigm. At the time of writing these flags are not
a finalized part of the UTFS logic, and should be used with caution.

**`UTFS_LOAD_EXPLICIT`**

Allows the system to load a specific file on demand via `utfs_load_file()`, rather than during
the usual `utfs_load()` call that loads every file.

Use with caution: any call to `utfs_save()` writes the current RAM contents of all files to the
medium. If an explicit file has not been loaded yet when `utfs_save()` is called, its blank RAM
buffer can be written over the previously stored data.

**`UTFS_SAVE_EXPLICIT`**

Allows the system to save a specific file on demand via `utfs_save_file()` (or
`utfs_save_flush()`), rather than during the usual `utfs_save()` call that saves every file.

Use with caution: a call to `utfs_save()` followed by a restart can cause the in-RAM data of an
explicit file to never reach the medium.

A second corner case: if an explicit file grows after the overall structure has been written to
the medium, saving that file can overwrite the files stored after it. This happens because
`utfs_save_file()` uses the current size of the data rather than the size recorded in the
already-written structure. This must be discussed and addressed before the flag is approved as a
permanent part of UTFS.
