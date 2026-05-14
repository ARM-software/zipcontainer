#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>

#ifdef __cplusplus
#include <string>
#include <vector>
#endif

#include "zipc.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Diff utility ---

enum zipc_diff_kind
{
	ZIPC_DIFF_CONTENT,
	ZIPC_DIFF_ONLY_IN_FIRST,
	ZIPC_DIFF_ONLY_IN_SECOND,
};
typedef enum zipc_diff_kind zipc_diff_kind;

struct zipc_file_diff
{
	const char* name;
	enum zipc_diff_kind kind;
	uint64_t size_first;
	uint64_t size_second;
	uint64_t offset_first_diff; // if ZIPC_DIFF_CONTENT
};
typedef struct zipc_file_diff zipc_file_diff;

struct zipc_comparison
{
	/// Overall status. If this is not ZIPC_SUCCESS, then ignore all other members.
	enum zipc_status status;
	/// Number of entries in the following array.
	int count;
	/// Array of differences. Null if there were no differences.
	const struct zipc_file_diff* differences;
};
typedef struct zipc_comparison zipc_comparison;

/// Compare two zip files and list differences. You must call `zipc_compare_free()` on the
/// return pointer once you are done with the data.
const zipc_comparison* zipc_compare(const char* first, const char* second);

/// Convenience helper function to pretty print the contents of the `zipc_file_diff` array.
void zipc_print_zipc_file_diff(const zipc_comparison* differences, FILE* out);

void zipc_compare_free(const zipc_comparison* differences);

#ifdef __cplusplus
}

/// Return a sorted list of files in the ZIP container. If `startsWith` is not empty,
/// only paths with that prefix are returned. Returns an empty vector if the archive
/// cannot be opened.
std::vector<std::string> zipc_files(const std::string& zipname,	const std::string& startsWith = std::string());

/// Add `targetFile` from the filesystem to `zipname`, using `targetFile` as the path
/// inside the ZIP container. Creates `zipname` if needed, and fails if the packed path
/// already exists.
enum zipc_status zipc_add_file(const std::string& zipname, const std::string& targetFile);

/// Extract `targetFile` from `zipname`, using `targetFile` as both the packed path and
/// output filesystem path. Overwrites the output file if it already exists.
enum zipc_status zipc_extract_file(const std::string& zipname, const std::string& targetFile);
#endif
