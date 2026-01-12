#include "zipc.h"

#include <assert.h>
#include <string.h>

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	// Create zip file
	const char* zip_filename = "testfile.zip";
	zipc* z = zipc_open(zip_filename, "w");
	const char* internal_filename = "testcontent.txt";
	const char* content = "This is a piece of content";
	zipc_write(z, internal_filename, strlen(content), content);
	//TBD assert(zipc_filesize(z, internal_filename) == (ssize_t)strlen(content));
	zipc_close(z);

	// Read zip file just created
	z = zipc_open("testfile.zip", "r");
	//TBD assert(zipc_filesize(z, internal_filename) == (ssize_t)strlen(content));
	char readback[128];
	memset(readback, 0, sizeof(readback));
	zipc_read(z, internal_filename, strlen(content), readback);
	//TBD assert(strncmp(content, readback, strlen(content)) == 0);
	char* m = (char*)zipc_map(z, internal_filename, "r");
	//TBD assert(strncmp(content, m, strlen(content)) == 0);
	zipc_unmap(z, m);
	zipc_close(z);

	// TBD test zipc_map with write mode

	// TBD test zipc_stream_*() calls
	//zipcstream* zipc_stream_open(zipc* handle, const char* path, const char* mode);
	//int zipc_stream_write(zipcstream* handle, const char* path, size_t size, void* ptr);
	//int zipc_stream_close(zipcstream* handle);
}
