# RetractorDB

[![CircleCI](https://circleci.com/gh/michalwidera/retractordb.svg?style=svg)](https://circleci.com/gh/michalwidera/retractordb)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

[comment]: # (VSCode view: Ctrl+k,v)

**RetractorDB** is a specialized time series database system designed for processing and recording regular time series data in real-time. The system is optimized for continuous data streams, signal processing, telemetry, and IoT monitoring applications.

> ⚠️ **This is work in progress:** Active development is ongoing.

## Overview

RetractorDB provides a unique approach to time series data management through continuous query processing and real-time data transformation. The system consists of three cooperating programs written in C++ and tested on x64 and ARM64 platforms under Linux.

**Key characteristics:**
- **Real-time processing**: Designed to operate with real-time constraints
- **Deterministic execution**: Sequential, deterministic processing thread
- **Continuous queries**: Queries execute continuously from declaration until system termination
- **Memory-efficient**: Uses shared memory for inter-process communication
- **Stream-oriented**: Built on a custom algebra for regular time series

### Project Links
- **Official website**: [retractordb.com](https://retractordb.com)
- **Documentation**: [gitbook-rdb](https://retractordb.gitbook.io/retractordb-docs/) (Comprehensive documentation in Polish)
- **Repository**: [github.com/michalwidera/retractordb](https://github.com/michalwidera/retractordb)

## Core Components

RetractorDB consists of three main programs:

### xretractor
The primary database process that compiles and executes query plans. It manages two threads:
- **Processing thread**: Sequential and deterministic query plan execution
- **Communication thread**: Handles external world communication and queue management

Features:
- Compiles RetractorQL query language (.rql files)
- Executes continuous queries
- Generates artifacts (binary data files)
- Supports visualization of query execution plans

### xqry
Database client for querying and controlling the xretractor process. Communicates via shared memory.

Features:
- Stream data retrieval
- Ad-hoc query execution
- Multiple output formats (raw, Graphite, InfluxDB, gnuplot)
- Server status and control
- Real-time data access

### xtrdb
Data accessor and testing tool for analyzing binary artifacts and metadata.

Features:
- Binary file inspection
- Metadata analysis
- Record reading and manipulation
- Interactive terminal interface
- Testing support

## Key Features

- **RetractorQL**: Custom query language based on ANTLR4 for stream processing
- **Stream Algebra**: Operations including interleaving, sum/difference, time shifting, aggregation/serialization
- **Continuous Queries**: Declare streams and transformations that execute continuously
- **Ad-hoc Queries**: Dynamic queries to running system without restart
- **Alarm System**: Rule-based monitoring with conditional triggers and actions
- **Multiple Data Types**: Support for BYTE, INTEGER, UINT, RATIONAL, FLOAT, DOUBLE, STRING
- **Type Inference**: Automatic type promotion during compilation
- **Retention Policies**: Configurable data retention and rotation
- **Signal Processing**: Built-in support for filtering and digital signal processing
- **Visualization**: Query plan visualization and real-time plotting with gnuplot
- **Shared Memory IPC**: Efficient inter-process communication
- **Deterministic**: Reproducible results for the same input data

## Installation

### Prerequisites

Install required system packages:

```bash
sudo apt-get -y install gcc cmake make build-essential python3 python3-pip python3-venv \
                        valgrind clang-format mold
```

Install Python packages (optional: use virtual environment):

```bash
# Optional: Create and activate virtual environment
python3 -m venv .venv
source .venv/bin/activate

# Install Conan and CMake formatter
pip install conan cmakelang
```

### Build and Install

1. **Configure Conan profile:**
```bash
conan profile detect
```

2. **Clone repository:**
```bash
git clone https://github.com/michalwidera/retractordb.git
cd retractordb
```

3. **Build with Conan:**
```bash
conan install . -s build_type=Debug --build missing
conan build . -s build_type=Debug --build missing
cd build/Debug
make install
```

4. **Verify installation:**
```bash
xqry -h
```

You should see the xqry help message.

### Optional Build Commands

In the `build/Debug` folder:

```bash
make test          # Run test suite
cmake .            # Reconfigure
make               # Build
make install       # Install binaries to ~/.local/bin
make descgrammar   # Build descriptor grammar
make rqlgrammar    # Build RQL grammar
make cformat       # Format code
```

### Optional: Visualization Support

For graphical output and diagrams:

```bash
sudo apt install gnuplot graphviz feh tmux
```

> **Note**: Binaries are installed to `~/.local/bin` - ensure this is in your PATH.

> **Note**: ANTLR4 grammar work requires Java, which is installed automatically via Conan.

### Alternative: Using Ninja Build System

[Ninja](https://ninja-build.org/manual.html) can be used as an alternative to Make for faster builds.

1. **Install Ninja:**
```bash
sudo apt install ninja-build
```

2. **Configure Conan profile** (edit `~/.conan2/profiles/default`):

```ini
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=gnu23
compiler.libcxx=libstdc++11
compiler.version=13
os=Linux

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
```

3. **Build with Ninja:**
```bash
# Remove previous build directory if it exists
rm -rf build/Debug

# Install and build
conan install . -s build_type=Debug --build missing
conan build . -s build_type=Debug --build missing
cd build/Debug

# Use Ninja commands
ninja install
ninja test
ninja descgrammar
ninja rqlgrammar
ninja cformat
```

## Query Language (RetractorQL)

RetractorDB uses a custom query language for declaring and processing time series streams. Example queries are located in the `examples/` directory as `*.rql` files.

### Query Types

#### 1. DECLARE - Stream Declaration

Declares a time series data source:

```sql
DECLARE field_name type [, field_name type ...]
STREAM stream_name, frequency
FILE source_path
[DISPOSABLE]
[ONESHOT]
[HOLD]
```

**Example** - Reading from /dev/random at 10 Hz:
```sql
DECLARE random_value INTEGER
STREAM random_stream, 0.1
FILE '/dev/random'
```

**Example** - Reading from text file:
```sql
DECLARE sensor_data INTEGER
STREAM sensor_input, 1
FILE 'datafile.txt'
```

#### 2. SELECT - Stream Processing

Creates continuous queries that transform and combine streams:

```sql
SELECT column_expression [AS column_name], [column_expression [AS column_name] ...]
STREAM output_stream_name
FROM stream_junction_expression
[FILE artifact_filename]
[RETENTION capacity [segments]]
[VOLATILE]
```

**Example** - Simple transformation:
```sql
SELECT core0[0] + 1
STREAM str1
FROM core0
```

**Example** - Combining streams:
```sql
SELECT str2[0]/2+1, str2[1]
STREAM str2
FROM core1+core0
```

#### 3. RULE - Alarm System

Defines conditional monitoring rules:

```sql
RULE rule_name
FROM stream_name
IF logical_condition
THEN action
[DUMP time_before:time_after]
```

**Example** - Trigger on threshold:
```sql
RULE alarm_high
FROM sensor_stream
IF sensor_stream[0] > 11
THEN DUMP 5:5
```

### Stream Algebra Operations

- **Interleaving** (`+`): Combines two streams by alternating their elements
- **Sum/Difference** (`+`, `-`): Mathematical operations on stream elements
- **Time Shifting** (`[n]`): Access past values (e.g., `stream[2]` - 2 samples ago)
- **Aggregation** (`@`): Moving window operations (e.g., `stream@10:100` - window of 100, step 10)
- **Serialization**: Flattening aggregated windows

### Supported Data Types

| Type | Size | Range | Description |
|------|------|-------|-------------|
| BYTE | 1 byte | 0..255 | 8-bit unsigned |
| INTEGER | 4 bytes | -2³¹ to 2³¹-1 | Signed integer |
| UINT | 4 bytes | 0 to 2³²-1 | Unsigned integer |
| RATIONAL | variable | - | Rational numbers |
| FLOAT | 4 bytes | - | Single precision float |
| DOUBLE | 8 bytes | - | Double precision float |
| STRING | declared | - | Fixed-size byte array |

## Quick Start Example

1. **Create a data source file:**
```bash
seq 1 100 > data.txt
```

2. **Create a query file** (`example.rql`):
```sql
-- Declare input stream
DECLARE value INTEGER
STREAM input, 1
FILE 'data.txt'

-- Process stream
SELECT value[0] * 2 AS doubled
STREAM output
FROM input
FILE 'output.bin'
```

3. **Run the query:**
```bash
# Terminal 1: Start processing
xretractor example.rql

# Terminal 2: View output
xqry -s output
```

4. **View query plan:**
```bash
xretractor example.rql -c -d
```

## Use Cases

- **Signal Processing**: Real-time filtering, FFT, signal analysis
- **IoT Monitoring**: Telemetry data processing and aggregation
- **Medical Monitoring**: Continuous patient vital signs processing
- **Industrial Automation**: Sensor data fusion and analysis
- **Time Series Analysis**: Moving windows, aggregations, pattern detection
- **Alarm Systems**: Rule-based event detection and triggering

## Architecture

- **Main Process** (xretractor): Dual-threaded - deterministic processing + communication
- **Shared Memory**: IPC mechanism for efficient data transfer
- **Binary Artifacts**: Efficient storage format with metadata
- **Continuous Execution**: Queries run indefinitely until termination
- **Type System**: Compile-time type inference and promotion

## Documentation

For comprehensive documentation (in Polish), see:
- [GitBook Documentation](https://retractordb.gitbook.io/retractordb-docs/)
- [Official Website](https://retractordb.com)

### Additional Resources
- [Presentation Slides (PL)](https://retractordb.com/assets/presentation/Prezentacja_RetractorDB_PL.pdf)
- [Interlace Example (Online)](https://retractordb.com/assets/interlace.html)
- [Sum Example (Online)](https://retractordb.com/assets/sum.html)

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Author

Created and maintained by **Michal Widera** (2003-2026)  
Contact: michal@widera.com.pl
