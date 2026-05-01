# RetractorDB

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CircleCI](https://dl.circleci.com/status-badge/img/circleci/JNsMwQGBP38ArHVd2CGwAF/WU9ujc7GeVXvtLjU2b5mSf/tree/master.svg?style=shield)](https://dl.circleci.com/status-badge/redirect/circleci/JNsMwQGBP38ArHVd2CGwAF/WU9ujc7GeVXvtLjU2b5mSf/tree/master)

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

- xretractor
- xqry
- xtrdb

## Installation

Check [cluade.md](CLAUDE.md)

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
