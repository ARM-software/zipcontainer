#include "zipc.h"

#include <string>

#include <string.h>

// Private definitions

struct zipc
{
	std::string filename;
	std::string mode;
};

struct zipcstream
{
	zipc* parent = nullptr;
};

// Implementations

zipc* zipc_open(const char* filename, const char* mode)
{
	zipc* z = new zipc();
	z->filename = filename;
	z->mode = mode;
	// TBD
	return z;
}

void zipc_close(zipc* handle)
{
	delete handle;
}

ssize_t zipc_filesize(zipc* handle, const char* path)
{
	// TBD
	return 0;
}

void* zipc_map(zipc* handle, const char* path, const char* mode)
{
	// TBD
	return nullptr;
}

void zipc_unmap(zipc* handle, void* ptr)
{
	// TBD
}

int zipc_write(zipc* handle, const char* path, size_t size, const void* ptr)
{
	// TBD
	return 0;
}

int zipc_read(zipc* handle, const char* path, size_t size, void* ptr)
{
	// TBD
	return 0;
}

zipcstream* zipc_stream_open(zipc* handle, const char* path, const char* mode)
{
	zipcstream* c = new zipcstream();
	// TBD
	return c;
}

int zipc_stream_write(zipcstream* handle, const char* path, size_t size, const void* ptr)
{
	// TBD
	return 0;
}

int zipc_stream_close(zipcstream* handle)
{
	// TBD use sendfile() to efficiently move temporary file into ZIP
	delete handle;
	return 0;
}
