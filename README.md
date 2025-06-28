# UTFS a TAR-like Embedded File System 

### Abstract

This repository is for the UTFS file system, a lightweight storage system with named 'files', designed for use in embedded systems.  UTFS is designed for flat address space, sequential memories such as EEPROM or Flash.  UTFS is based on concepts from the TAR archive format where data is written sequentially without gaps.

### Repository Contents

- /Examples - This folder contains multiple projects for UTFS. Systems include SAMD20, Arduino, Linux, etc.
- /src - This folder is the source code for the UTFS reference implementation
- /src_minimum - This folder contains a 'minimized' version of UTFS, just the basics to read and write files

### Background

Microcontroller embedded systems are commonly developed as ‘bare metal’ solutions without operating system or built in libraries.  While many embedded systems do not have an operating system, it is common for the the systems to need to store and retrieve data across power cycles. This lack of system facilities leave the design of data storage to the developer.

Existing file systems are designed for humans and not embedded machines.  Long file name support, directory hierarchy, permission settings, and access attributes are common in modern file systems. Microcontrollers and embedded systems do not typically need these human constructs, but these features can not typically be easily disabled in established file systems. 

Data storage mediums in microcontroller systems commonly involve EEPROM, CPU flash pages, or external SPI/I2C flash chips.  These chips provide a flat, sequential address space to store arbitrary data. Microcontroller systems data storage solutions are commonly based on ‘raw’ data reads and write to these memories, data is loaded at start, and stored as needed in a fixed format.  The limitations to this are that the format is difficult to change, and commonly the system must be designed around the data storage layout.  Adding additional data after initial release typically results in an incompatible storage layout.  This could lead to data corruption, data loss, or a break backward compatibility with legacy systems. 

The UTFS file system is designed to address the shortcomings in resource constrained system systems without modern file systems, provide a developer friendly interface which allows for future expansion, while maintaining a minimal code and RAM footprint. 

### Goals
The design goals of UTFS are:

- Read/write arbitrary data to flat storage devices
- Provide a mechanism for accessing data in 'files'
- Allow file size to change without data loss
- Provide an application agnostic interface
- Low code and RAM footprint
- Minimize the number of reads and writes to the medium
- Unencumbered by proprietary or undocumented vendor formats
- Unencumbered by patents
- Agnostic to underlying memory architecture

# Theory of Operation


The UTFS file system was inspired from the structure of the TAR file formats developed to store files on tape archive.

### TAR

In the TAR file system, a file has a 512byte header with all file information including size, filename, user, group, and permission settings. Later versions of TAR (PAX standard) also have an optional extended header section. 

After the header, raw file data is written in 512 byte blocks. If a file data does not fill an entire 512 byte block, the block is padded up to 512 bytes.

This header+data structure is repeated with the next file in the archive.  This allows an arbitrary number of files to be stored in the TAR archive

### UTFS Design

Resource constrained systems may not have the ability to load and store 512 byte blocks.  Additionally, embedded systems commonly do not need the TAR header user, group, permissions, and extended settings, resulting in wasted space on the storage medium.

The UTFS system implements a 24 byte header with information about the ‘file’, including file system identifier, feature flags, file signature, data size, and a string for reference in the system.

After the header, data is written, up to the size specified in the header.

# UTFS Data Structure

```
       | Byte0  | Byte1  | Byte2  | Byte3  |

       -------------------------------------
Word 0 |    Identifier   |Version | Flags  |
       -------------------------------------
Word 1 |                Size               |
       -------------------------------------
Word 2 |   Signature     |    Reserved     |
       -------------------------------------
Word 3 |              Filename             |
Word 4 |                                   |
Word 5 |                                   |
       -------------------------------------
```
Total header size: 24 bytes

### UTFS Header Fields

| Name|Size| Index |  Description|
| --- | --- | --- | ---|
| Identifier|2 bytes | 0 | Identifier for file format, constant, 0x1984|
| Version  |1 byte | 2 | Currently 1|
| Flags |1 byte | 3 | Flags for features of the file |
| Size     |4 bytes | 4 | Size in bytes of 'data block'|
| Signature |2 bytes | 8 | Signature value for file, set by application |
| Reserved |2 bytes | 10 | |
| Filename |12 bytes | 12 | Human readable string to associate data|



# General Information


### Endianness

The default endianness of the data in a UTFS header shall be little-endian

### Naming

The UTFS name stands for "Micro TAR File System", where the U is the SI unit for 10^-6, aka micro.

The system name shall officially be printed in all uppercase letters; UTFS.

When appropriate the U may be lowercase or replaced with Mu, to indicate the small nature of the file system; uTFS and μTFS

### Credits

- Andrew Gaylo - CLI Systems - admin@clisystems.com
