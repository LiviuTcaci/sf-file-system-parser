# Section File System (SF Parser)

A C implementation of a file system module that processes a custom binary format called SF (Section File), with capabilities for file parsing, directory traversal, and content extraction.

## Overview

This project implements a parser and processor for SF (Section File) format, a binary file format that organizes data into sections with specific headers. The implementation includes commands for:

- Listing directory contents with filtering options
- Parsing and validating SF files
- Extracting specific lines from sections within SF files
- Finding SF files that match specific criteria

## File Format Specification

### SF File Structure

```
├── HEADER
│   ├── MAGIC (1 byte)
│   ├── HEADER_SIZE (2 bytes)
│   ├── VERSION (2 bytes)
│   ├── NO_OF_SECTIONS (1 byte)
│   └── SECTION_HEADERS
│       ├── SECT_NAME (20 bytes)
│       ├── SECT_TYPE (4 bytes)
│       ├── SECT_OFFSET (4 bytes)
│       └── SECT_SIZE (4 bytes)
└── BODY
    ├── Section 1
    ├── Section 2
    └── ...
```

### Format Rules

- MAGIC field is "s"
- VERSION must be between 42 and 101
- NO_OF_SECTIONS must be either 2 or between 5 and 20
- Valid SECT_TYPE values are: 88, 23, 60 (base 10)
- Lines within sections are separated by "\n" (0x0A)
- Lines are numbered from the end to the beginning (starting with 1)
- Lines are read in reversed order (from last character to first)

## Commands

### Display Variant
```
./a1 variant
```
Displays the assignment variant number.

### List Directory Contents
```
./a1 list [recursive] [filtering_options] path=<dir_path>
```
Lists files and directories at the specified path with optional filtering.

#### Filtering Options:
- `name_ends_with=<suffix>`: Only items ending with the specified suffix
- `has_perm_write`: Only items with write permission for the owner

### Parse SF File
```
./a1 parse path=<file_path>
```
Validates an SF file and displays information about its sections if valid.

### Extract Section Line
```
./a1 extract path=<file_path> section=<sect_nr> line=<line_nr>
```
Extracts a specific line from a section in an SF file.

### Find SF Files
```
./a1 findall path=<dir_path>
```
Recursively finds all SF files that have at least 4 sections of type 60.

## Building

Compile the program with:
```
gcc -Wall a1.c -o a1
```

## Examples

### Parse Example
```
$ ./a1 parse path=example.sf
SUCCESS
version=45
nr_sections=2
section1: sect1 88 33
section2: sect2 23 12
```

### Extract Example
```
$ ./a1 extract path=example.sf section=1 line=2
SUCCESS
second line
```

### List Example
```
$ ./a1 list path=./
SUCCESS
./a1.c
./a1_data.json
./example.sf
./tester.py
```

## Testing

Run the included tester script to verify functionality:
```
python3 tester.py
```

Advanced testing options:
```
python3 tester.py --verbose     # For detailed test output
python3 tester.py --test parse_1 # To run a specific test
python3 tester.py --valgrind    # To check for memory leaks
```
