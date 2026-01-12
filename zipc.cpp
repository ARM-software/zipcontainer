#include "zipc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Private definitions

enum zipc_mode
{
	ZIPC_READ_ONLY,
	ZIPC_WRITE_ONLY,
	ZIPC_APPEND,
};

static uint16_t read_le16(const unsigned char* ptr)
{
	return (uint16_t)ptr[0] | ((uint16_t)ptr[1] << 8U);
}

static uint32_t read_le32(const unsigned char* ptr)
{
	return (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8U) | ((uint32_t)ptr[2] << 16U) | ((uint32_t)ptr[3] << 24U);
}

static bool write_empty_archive(FILE* fp)
{
	static const unsigned char eocd[22] = {0x50, 0x4b, 0x05, 0x06,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	if (fseek(fp, 0, SEEK_SET) != 0) return false;
	if (fwrite(eocd, 1, sizeof(eocd), fp) != sizeof(eocd)) return false;
	return fflush(fp) == 0;
}

static bool compute_data_offset(FILE* fp, uint32_t local_offset, size_t* data_offset)
{
	if (fseek(fp, local_offset, SEEK_SET) != 0) return false;
	unsigned char local[30];
	if (fread(local, 1, sizeof(local), fp) != sizeof(local)) return false;
	if (read_le32(local) != 0x04034b50) return false;
	const uint16_t name_len = read_le16(local + 26);
	const uint16_t extra_len = read_le16(local + 28);
	*data_offset = static_cast<size_t>(local_offset) + 30U + name_len + extra_len;
	return true;
}

struct filenode
{
	size_t size;
	size_t offset;
};

struct zipc
{
	std::string filename;
	zipc_mode mode;
	FILE* fp = nullptr;
	std::unordered_map<std::string, filenode> files;
};

struct zipcstream
{
	zipc* parent = nullptr;
};

// Implementations

static enum zipc_status load_existing_archive(zipc* z)
{
	FILE* fp = z->fp;
	if (fseek(fp, 0, SEEK_END) != 0) return ZIPC_IO_FAILURE;
	const long file_size = ftell(fp);
	if (file_size < 0) return ZIPC_IO_FAILURE;
	if (file_size == 0) return ZIPC_SUCCESS;
	if (file_size < 22) return ZIPC_CORRUPT_ARCHIVE;

	const size_t max_search = 0xFFFF + 22;
	size_t search = static_cast<size_t>(file_size);
	if (search > max_search) search = max_search;

	std::vector<unsigned char> tail(search);
	if (fseek(fp, file_size - static_cast<long>(search), SEEK_SET) != 0) return ZIPC_IO_FAILURE;
	if (fread(tail.data(), 1, search, fp) != search) return ZIPC_IO_FAILURE;

	ssize_t eocd_idx = -1;
	for (size_t i = search - 22;; --i)
	{
		if (read_le32(&tail[i]) == 0x06054b50)
		{
			eocd_idx = static_cast<ssize_t>(i);
			break;
		}
		if (i == 0) break;
	}
	if (eocd_idx < 0) return ZIPC_CORRUPT_ARCHIVE;

	const long eocd_pos = file_size - static_cast<long>(search) + eocd_idx;
	const unsigned char* eocd = tail.data() + eocd_idx;
	const uint16_t entry_count = read_le16(eocd + 10);
	const uint32_t cd_size = read_le32(eocd + 12);
	const uint32_t cd_offset = read_le32(eocd + 16);
	if (cd_offset + cd_size > static_cast<uint32_t>(eocd_pos)) return ZIPC_CORRUPT_ARCHIVE;

	if (fseek(fp, cd_offset, SEEK_SET) != 0) return ZIPC_IO_FAILURE;
	std::vector<unsigned char> header(46);
	for (uint16_t i = 0; i < entry_count; ++i)
	{
		if (fread(header.data(), 1, header.size(), fp) != header.size()) return ZIPC_CORRUPT_ARCHIVE;
		if (read_le32(header.data()) != 0x02014b50) return ZIPC_CORRUPT_ARCHIVE;

		const uint16_t flags = read_le16(header.data() + 8);
		const uint16_t method = read_le16(header.data() + 10);
		const uint32_t compressed_size = read_le32(header.data() + 20);
		const uint32_t uncompressed_size = read_le32(header.data() + 24);
		const uint16_t name_len = read_le16(header.data() + 28);
		const uint16_t extra_len = read_le16(header.data() + 30);
		const uint16_t comment_len = read_le16(header.data() + 32);
		const uint32_t local_offset = read_le32(header.data() + 42);

		if (method != 0) return ZIPC_UNSUPPORTED_FEATURE;
		if (flags & 0x08) return ZIPC_UNSUPPORTED_FEATURE;
		if (compressed_size != uncompressed_size) return ZIPC_UNSUPPORTED_FEATURE;

		std::string name(name_len, '\0');
		if (fread(name.data(), 1, name_len, fp) != name_len) return ZIPC_CORRUPT_ARCHIVE;
		if (fseek(fp, extra_len + comment_len, SEEK_CUR) != 0) return ZIPC_CORRUPT_ARCHIVE;

		size_t data_offset = 0;
		const long return_pos = ftell(fp);
		if (!compute_data_offset(fp, local_offset, &data_offset)) return ZIPC_CORRUPT_ARCHIVE;
		if (fseek(fp, return_pos, SEEK_SET) != 0) return ZIPC_IO_FAILURE;

		if (z->files.count(name) == 0)
		{
			z->files.emplace(std::move(name), filenode{static_cast<size_t>(uncompressed_size), data_offset});
		}
	}
	return ZIPC_SUCCESS;
}

zipc* zipc_open(const char* filename, const char* mode, enum zipc_status* err)
{
	if (err) *err = ZIPC_SUCCESS;
	if (!filename || !mode || strlen(mode) != 1)
	{
		if (err) *err = ZIPC_SYNTAX_ERROR;
		return nullptr;
	}

	zipc* z = new zipc();
	z->filename = filename;
	if (mode[0] == 'r') z->mode = ZIPC_READ_ONLY;
	else if (mode[0] == 'w') z->mode = ZIPC_WRITE_ONLY;
	else if (mode[0] == 'a') z->mode = ZIPC_APPEND;
	else
	{
		if (err) *err = ZIPC_SYNTAX_ERROR;
		delete z;
		return nullptr;
	}

	const char* fopen_mode = (z->mode == ZIPC_READ_ONLY) ? "rb" : (z->mode == ZIPC_WRITE_ONLY ? "wb+" : "rb+");
	z->fp = fopen(filename, fopen_mode);
	if (!z->fp && z->mode == ZIPC_APPEND)
	{
		z->fp = fopen(filename, "wb+");
		if (z->fp && !write_empty_archive(z->fp))
		{
			fclose(z->fp);
			z->fp = nullptr;
		}
	}
	if (!z->fp)
	{
		if (err) *err = ZIPC_PERMISSION_FAILURE;
		delete z;
		return nullptr;
	}

	if (z->mode == ZIPC_WRITE_ONLY)
	{
		if (!write_empty_archive(z->fp))
		{
			if (err) *err = ZIPC_IO_FAILURE;
			fclose(z->fp);
			delete z;
			return nullptr;
		}
		return z;
	}

	const enum zipc_status status = load_existing_archive(z);
	if (status != ZIPC_SUCCESS)
	{
		if (err) *err = status;
		fclose(z->fp);
		delete z;
		return nullptr;
	}

	if (z->mode == ZIPC_APPEND) fseek(z->fp, 0, SEEK_END);
	return z;
}

void zipc_close(zipc* handle)
{
	if (!handle) return;
	if (handle->fp) fclose(handle->fp);
	delete handle;
}

ssize_t zipc_filesize(zipc* handle, const char* path)
{
	assert(handle);
	assert(path);
	if (handle->files.count(path) == 0) return -1;
	const auto& v = handle->files.at(path);
	return v.size;
}

const void* zipc_map_read(zipc* handle, const char* path, enum zipc_status* err)
{
	assert(handle);
	assert(path);
	// TBD
	if (err) *err = ZIPC_SUCCESS;
	return nullptr;
}

void* zipc_map_write(zipc* handle, const char* path, enum zipc_status* err, size_t max)
{
	assert(handle);
	assert(path);
	// TBD
	if (err) *err = ZIPC_SUCCESS;
	return nullptr;
}

void zipc_unmap(zipc* handle, void* ptr)
{
	assert(handle);
	// TBD
}

enum zipc_status zipc_write(zipc* handle, const char* path, size_t size, const void* ptr)
{
	assert(handle);
	assert(path);
	// TBD
	return ZIPC_SUCCESS;
}

enum zipc_status zipc_read(zipc* handle, const char* path, size_t size, void* ptr)
{
	assert(handle);
	assert(path);
	// TBD
	return ZIPC_SUCCESS;
}

zipcstream* zipc_stream_open(zipc* handle, const char* path, const char* mode, enum zipc_status* err)
{
	assert(handle);
	assert(path);
	zipcstream* c = new zipcstream();
	// TBD
	if (err) *err = ZIPC_SUCCESS;
	return c;
}

enum zipc_status zipc_stream_write(const zipc* handle, zipcstream* stream, const char* path, size_t size, const void* ptr)
{
	assert(handle);
	assert(stream);
	// TBD
	return ZIPC_SUCCESS;
}

enum zipc_status zipc_stream_close(zipc* handle, zipcstream* stream)
{
	assert(handle);
	assert(stream);
	// TBD use sendfile() to efficiently move temporary file into ZIP
	delete stream;
	return ZIPC_SUCCESS;
}
