# LOG_SOURCE Library

A comprehensive C++17 library for working with log files, providing reading, writing, filtering, pattern matching, and change detection capabilities.

## Features

- 📖 **Read & Write**: Open log files in READ or WRITE mode
- 🔍 **Filtering**: Filter logs by level, time range, message, and metadata
- 🎯 **Pattern Matching**: Regex-based pattern matching across log entries
- 🔔 **Change Detection**: Monitor log files for updates with `hasUpdated()`
- 💾 **Memory-Mapped I/O**: Efficient handling of large files (>1MB)
- 🎨 **Polymorphic Container**: LOG_DATA adapts to single or multiple records
- ⚡ **High Performance**: Filter 10K records in <100ms
- 🛡️ **Error Handling**: Explicit error propagation with Result<T> pattern
- 🌐 **Cross-Platform**: Windows, Linux, and macOS support

## Quick Start

### Reading Logs

```cpp
#include "stdlib/source/log_source.hpp"

using namespace ams::stdlib::source;

// Open log file
auto result = LOG_SOURCE::open("/var/log/app.log", AccessMode::READ);
if (result.isOk()) {
    auto logs = result.unwrap();
    
    // Filter by ERROR level
    LOG_DATA errors = logs.filter(FilterCriteria::byLevel(LogLevel::ERROR));
    
    std::cout << "Found " << errors.count() << " errors" << std::endl;
}
```

### Writing Logs

```cpp
// Open for writing
auto logs = LOG_SOURCE::open("/var/log/app.log", AccessMode::WRITE).unwrap();

// Create log record
LOG_RECORD record{
    std::chrono::system_clock::now(),
    LogLevel::INFO,
    "Application started",
    {{"version", "1.0.0"}}
};

// Insert record
logs.insert(record);
```

### Monitoring Changes

```cpp
auto logs = LOG_SOURCE::open("/var/log/app.log", AccessMode::READ).unwrap();

while (true) {
    if (logs.hasUpdated()) {
        std::cout << "Log file updated!" << std::endl;
        // Process new entries...
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
}
```

## Components

### LOG_SOURCE
Main interface for log file operations. Provides methods for opening, reading, writing, filtering, and monitoring log files.

### LOG_DATA
Polymorphic container that dynamically holds:
- Empty (count = 0)
- Single record (count = 1)
- Multiple records (count > 1)

### LOG_RECORD
Immutable structure representing a single log entry:
- `timestamp`: ISO 8601 with milliseconds
- `level`: DEBUG, INFO, WARN, ERROR, FATAL
- `message`: Log message content
- `metadata`: Optional key-value pairs

### FilterCriteria
Builder pattern for creating complex filters:
```cpp
FilterCriteria criteria = FilterCriteria::byLevel(LogLevel::ERROR)
    .andMessage("Connection")
    .andMetadata("host", "db.example.com");
```

## File Structure

```
include/stdlib/source/
├── log_source.hpp          # Main interface
├── log_file.hpp            # File I/O abstraction
├── log_data.hpp            # Polymorphic container
├── log_record.hpp          # Log record structure
├── parser.hpp              # Log parser
├── pretty_printer.hpp      # Log formatter
├── filter_engine.hpp       # Filtering engine
├── pattern_matcher.hpp     # Regex matcher
├── filter_criteria.hpp     # Filter builder
└── error.hpp               # Error handling
```

## Log Format

Standard format:
```
[TIMESTAMP] [LEVEL] MESSAGE [METADATA]
```

Example:
```
2024-01-15T10:30:45.123Z INFO Application started
2024-01-15T10:30:46.456Z ERROR Connection failed host=db.example.com port=5432
```

## Building

### Requirements
- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+ (optional)

### Compile Example
```bash
g++ -std=c++17 -I. your_program.cpp -o your_program
```

## Testing

Run all tests:
```bash
# Filter Engine tests
./build/filter_engine_test.exe

# Pattern Matcher tests
./build/pattern_matcher_test.exe

# LOG_DATA integration tests
./build/log_data_integration_test.exe

# LOG_FILE tests
./build/log_file_test.exe

# LOG_SOURCE tests
./build/log_source_test.exe
```

All tests should pass with 100% success rate.

## Performance

- **File Size**: Optimized for files up to 1 GB
- **Filter**: 10,000 records in < 100ms
- **Pattern Match**: 10,000 records in < 200ms
- **Change Detection**: < 10ms
- **Memory**: Memory-mapped I/O for files > 1MB

## Documentation

- [API Documentation](../../../docs/LOG_SOURCE_API.md)
- [Examples](../../../examples/)

## Examples

See the `examples/` directory for complete AMS programs:
- `log_source_basic_read.ams` - Basic log reading
- `log_source_pattern_matching.ams` - Pattern matching
- `log_source_change_detection.ams` - Change monitoring
- `log_source_write_example.ams` - Log writing
- `log_source_complex_filtering.ams` - Advanced filtering

## Error Handling

All operations that can fail return `Result<T>`:

```cpp
auto result = LOG_SOURCE::open("file.log", AccessMode::READ);

if (result.isOk()) {
    auto logs = result.unwrap();
    // Use logs...
} else {
    std::cerr << "Error: " << result.getErrorMessage() << std::endl;
}
```

## Thread Safety

⚠️ **Not thread-safe**. For concurrent access:
- Use separate LOG_SOURCE instances per thread for reading
- Synchronize write operations with mutexes

## Platform Support

- ✅ Windows (Windows API for memory-mapped I/O)
- ✅ Linux (POSIX APIs)
- ✅ macOS (POSIX APIs)

## License

Part of the AMS language project.

## Contributing

See the main AMS project for contribution guidelines.
