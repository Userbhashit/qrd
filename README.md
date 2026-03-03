# qrd - Quick Registry Document

A CLI document registry manager that allows users to store, list, open, and remove document references with a configurable viewer command.

> **Note**: This was a learning project. I utilized an AI agent (Cursor) specifically for the TUI table-rendering logic in list_documents to ensure a consistent visual experience. All core logic including path resolution, memory safety, and configuration handling was manually implemented.

## Features

- Add document references with custom aliases
- Open documents using your preferred application
- List all stored documents (with optional type filter)
- Remove document entries
- Configurable viewer command (supports any CLI tool: `open`, `xdg-open`, `cat`, etc.)

## Build

```bash
mkdir build && cd build
cmake ..
make
```

The executable will be at `build/qrd`.

### Debug Build (with AddressSanitizer)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Usage

| Flag | Description | Example |
|------|-------------|---------|
| `-a <type> <alias> <path>` | Add a document | `./qrd -a pdf notes /path/to/file.pdf` |
| `-o <alias>` | Open a document | `./qrd -o notes` |
| `-l [type]` | List documents | `./qrd -l` or `./qrd -l pdf` |
| `-d <alias>` | Remove a document | `./qrd -d notes` |
| `-s <command>` | Set viewer command | `./qrd -s open` |
| `-h` | Show help | `./qrd -h` |

## First-Time Setup

Before opening documents, you must set your preferred viewer command:

```bash
# macOS
./qrd -s open

# Linux
./qrd -s xdg-open

# Or any other command (cat, less, etc.)
./qrd -s cat
```

This saves the command to `~/.config/qrd/command`.

## Example Workflow

```bash
# Set your viewer command (one-time)
./qrd -s open

# Add some documents
./qrd -a pdf notes ./notes.pdf
./qrd -a img screenshot ./screenshot.png

# List all documents
./qrd -l

# List only PDFs
./qrd -l pdf

# Open a document
./qrd -o notes

# Remove a document
./qrd -d screenshot
```

## Storage

All data is stored in `~/.config/qrd/`:
- `registry` - Document aliases and paths
- `command` - Your configured viewer command

## Requirements

- C99 compatible compiler
- CMake 3.10+
- macOS or Linux
