# File system

The Half-Life Unified SDK uses the Half-Life engine's `IFileSystem` interface for nearly all filesystem operations.

Helper functions in `utils/filesystem_utils.h` should be used to perform common tasks like reading a file into a buffer or writing text to a file.

The `FSFile` class can be used to access files that are automatically closed when the file object goes out of scope.
