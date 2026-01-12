#pragma once

#include <stdio.h>

struct zipc;
struct zipcstream;

/// Open an uncompressed ZIP file. Mode is 'r' for reading, 'w' for writing (replacing
/// any existing file), or 'a' for appending.
zipc* zipc_open(const char* filename, const char* mode);

/// Close an open ZIP file handle.
void zipc_close(zipc* handle);

/// Obtain the filesize of a given file inside the ZIP container. Return -1 if the
/// file is not found.
ssize_t zipc_filesize(zipc* handle, const char* path);

/// Memory map a file inside the ZIP container for streaming read or write. Read or write
/// once and then unmap it. Mode must be either 'w' to write (path must not already exist
/// in container) or 'r' to read. You must not write to the ZIP file in any other way while
/// having a memory map to it.
void* zipc_map(zipc* handle, const char* path, const char* mode);

/// Unmap the memory mapped above.
void zipc_unmap(zipc* handle, void* ptr);

/// Create and write a new file inside the ZIP container. Returns zero on success,
/// a non-zero value if there was a terminal failure of some kind.
int zipc_write(zipc* handle, const char* path, size_t size, const void* ptr);

/// Read the given number of bytes from a file from inside the ZIP container into
/// the given pointer. Returns zero on success, a non-zero value if there was a
/// terminal failure of some kind.
int zipc_read(zipc* handle, const char* path, size_t size, void* ptr);

/// Open a stream for continous writing to a file inside a ZIP container that can be
/// used simultaneously with other read and write operations. Unlike all other calls
/// here, this is not guaranteed to be a zero-copy operation as we likely must use
/// temporary intermediate files. `mode` is reserved for future use.
zipcstream* zipc_stream_open(zipc* handle, const char* path, const char* mode);

/// Write to our stream. Returns zero on success, a non-zero value if there was a
/// terminal failure of some kind.
int zipc_stream_write(zipcstream* handle, const char* path, size_t size, const void* ptr);

/// Close the write stream.
int zipc_stream_close(zipcstream* handle);
