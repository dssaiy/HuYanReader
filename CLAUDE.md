# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

HuYanReader is a high-performance multi-threaded novel downloader built with Qt5 and C++. It supports multiple book sources, concurrent downloads, and features a complete SSL/TLS network stack with intelligent retry mechanisms.

### Architecture

The project follows a modular architecture with clear separation of concerns:

```
src/
├── core/           # Main application, document models, text management
├── ui/             # User interface components (Qt widgets, dialogs)
├── network/        # HTTP client with SSL/TLS, cookie management, retry logic
├── novel/          # Novel-specific business logic (search, download, file generation)
├── parser/         # HTML parsing using Lexbor, rule-based content extraction
└── config/         # Application settings and configuration dialogs
```

Key components:
- **MainWindow**: Central hub with system tray, hotkeys, and view management
- **NovelSearchManager**: Orchestrates search and download operations across multiple sources
- **HttpClient**: Thread-safe HTTP client with custom cookie jar and retry mechanisms
- **ChapterDownloader**: Multi-threaded chapter downloading with progress tracking
- **RuleManager + ContentParser**: Rule-based HTML parsing using Lexbor library

### Multi-threading Model

- **Main thread**: UI interaction and task coordination
- **Search threads**: Concurrent search across multiple book sources
- **Download thread pool**: Parallel chapter downloading
- **Parser threads**: HTML content extraction and processing

## Build Commands

### Prerequisites
- Qt 5.14.2 or higher
- CMake 3.21+
- Visual Studio 2019+ or MinGW-w64
- Lexbor HTML parser (included as submodule)

### Build Process

```bash
# Create and enter build directory
mkdir build_vs
cd build_vs

# Configure with CMake (Visual Studio)
cmake .. -G "Visual Studio 16 2019" -A x64

# Build the main application
cmake --build . --config Release --target ProtectEye

# Build all targets including tests
cmake --build . --config Release

# Deploy Qt dependencies
cmake --build . --config Release --target DeployQt
```

### Available Executables
- `ProtectEye.exe` - Main novel downloader application
- `BookSourceTest.exe` - Test utility for validating book sources
- `IntegrationTest.exe` - Integration test suite

### Running Tests
```bash
# Run book source tests
./bin/BookSourceTest.exe

# Run integration tests
./bin/IntegrationTest.exe
```

## Key Technical Details

### Book Source Configuration
Book sources are configured via JSON files in `resources/rules/`. Each source defines:
- Search API endpoints and parameters
- HTML selectors for extracting book metadata
- Chapter content parsing rules
- Pagination handling for multi-page chapters

Example structure:
```json
{
  "id": 1,
  "name": "Source Name",
  "search": {
    "url": "https://example.com/search",
    "method": "post",
    "result": "#results > .book"
  },
  "chapter": {
    "content": "#content",
    "filterTag": "script div.ad"
  }
}
```

### Network Layer Features
The HttpClient provides:
- Custom cookie jar with public access interface
- User-agent rotation for anti-bot protection
- Configurable timeout and retry mechanisms
- Thread-safe synchronous and asynchronous operations
- SSL/TLS support with certificate handling

### Threading Architecture
- Uses Qt's signal/slot mechanism for thread communication
- Thread-safe components with QMutex protection
- ChapterDownloader manages a pool of concurrent download tasks
- NovelSearchManager coordinates search operations across multiple sources

## Development Guidelines

### File Path Handling
This project runs on Windows. When working with file paths:
- Always use forward slashes (/) in code when possible
- The CMake build system handles path conversion automatically
- Generated executables are placed in `build_vs/bin/`

### Adding New Book Sources
1. Create JSON configuration in `resources/rules/`
2. Test with `BookSourceTest.exe`
3. Update rule loading logic if needed
4. Verify download functionality with integration tests

### Qt Integration
- Uses CMake's Qt5 integration with AUTOMOC, AUTOUIC, AUTORCC
- UI files are automatically processed by CMake
- MOC files are generated automatically for Q_OBJECT classes
- Resource files (.qrc) are compiled into the executable

### Lexbor HTML Parser
The project uses Lexbor as a git submodule for high-performance HTML parsing. The library is built as a static library and linked automatically by CMake.