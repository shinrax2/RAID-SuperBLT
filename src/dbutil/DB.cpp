#include "DB.h"

#include <util/util.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace blt::db;
using blt::idstring;

static_assert(sizeof(void*) == sizeof(intptr_t));
using pos_t = std::ios::pos_type;
using FileList = std::vector<DslFile>&;
using FileMap = std::map<std::pair<idstring, idstring>, DslFile*>&;

static uint64_t monotonicTimeMicros()
{
	// https://docs.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
	LARGE_INTEGER StartingTime;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartingTime);

	StartingTime.QuadPart *= 1000000;
	StartingTime.QuadPart /= Frequency.QuadPart;
	return StartingTime.QuadPart;
}

struct dsl_Vector
{
	unsigned int size;
	unsigned int capacity;
	void* allocator;
	intptr_t contents_ptr;
};

static void skipVector(std::istream& in)
{
	in.seekg(sizeof(dsl_Vector), std::ios::cur);

	// Suppress the unused function warning
	(void)skipVector;
}

template <typename T> static std::vector<T> loadVector(std::istream& in, int offset, dsl_Vector& vec)
{
	std::vector<T> data;
	data.resize(vec.size);

	// Read the vector contents
	pos_t pos = in.tellg();
	in.seekg(vec.contents_ptr + offset);
	in.read((char*)data.data(), sizeof(T) * data.size());
	in.seekg(pos);

	return data;
}

template <typename T> static std::vector<T> loadVector(std::istream& in, int offset)
{
	// Read the vector metadata
	dsl_Vector vec = {0};
	in.read((char*)&vec, sizeof(vec));

	return loadVector<T>(in, offset, vec);
}

static std::mutex db_setup_mutex;

DieselDB* DieselDB::Instance()
{
	// Make sure we can't go setting it up twice in parallel
	std::lock_guard guard(db_setup_mutex);

	static DieselDB instance;
	return &instance;
}

static void loadPackageHeader(std::string headerPath, std::string dataPath, FileList);
static void loadBundleHeader(std::string headerPath, std::string dataPath, FileList);

////////////////////////
////// DSL FILE ////////
////////////////////////

std::vector<uint8_t> DslFile::ReadContents(std::istream& fi) const
{
	unsigned int realLength = length;
	if (!HasLength())
	{
		// This is an end-of-file asset, so it's length is it's start until the end of the file
		fi.seekg(0, std::ios::end);
		realLength = (unsigned int)fi.tellg() - offset;
	}

	std::vector<uint8_t> data(realLength);
	fi.seekg(offset);
	fi.read((char*)data.data(), data.size());
	return data;
}

////////////////////////
////// DIESEL DB ///////
////////////////////////

DieselDB::DieselDB()
{
	uint64_t start_time = monotonicTimeMicros();
	RAIDHOOK_LOG_LOG("Start loading DB info");

	std::ifstream in;
	in.exceptions(std::ios::failbit | std::ios::badbit);

	// "Future" proofing.
	std::string blb_names[2] = {
		"all",
		"bundle_db"
	};

	std::string blb_suffix = ".blb";
	std::string blb_path;

	for (const std::string& name : raidhook::Util::GetDirectoryContents("assets"))
	{
		if (name.length() <= blb_suffix.size())
			continue;
		if (name.compare(name.size() - blb_suffix.size(), blb_suffix.size(), blb_suffix) != 0)
			continue;

		bool valid_name = false;
		for (const std::string& blb_name : blb_names) {
			if (name.compare(0, blb_name.size(), blb_name) == 0) {
				valid_name = true;
				break;
			}
		}

		if (!valid_name)
			continue;

		blb_path = name;
	}

	if (blb_path.empty()) {
		RAIDHOOK_LOG_ERROR("No 'all.blb' or 'bundle_db.blb' found in 'assets' folder, not loading asset database!");
		return;
	}

	in.open("assets/" + blb_path, std::ios::binary);

	// Skip a pointer - vtable or allocator probably?
	in.seekg(sizeof(void*), std::ios::cur);

	// Build out the LanguageID-to-idstring mappings
	struct LanguageData
	{
		idstring name;
		uint32_t id;
		uint32_t padding; // Probably padding, at least - always zero
	};
	static_assert(sizeof(LanguageData) == 16);
	std::map<int, idstring> languages;
	for (const LanguageData& lang : loadVector<LanguageData>(in, 0))
	{
		languages[lang.id] = lang.name;
	}

	// Sortmap
	in.seekg(sizeof(void*) * 2, std::ios::cur);
	in.seekg(sizeof(void*), std::ios::cur);

	// Files
	struct MiniFile
	{
		idstring type;
		idstring name;
		uint32_t langId;
		uint32_t zero_1;
		uint32_t fileId;
		uint32_t zero_2;
	};
	static_assert(sizeof(MiniFile) == 32); // Same on 32 and 64 bit
	std::vector<MiniFile> miniFiles = loadVector<MiniFile>(in, 0);
	filesList.resize(miniFiles.size());

	for (size_t i = 0; i < miniFiles.size(); i++)
	{
		MiniFile& mini = miniFiles[i];
		//printf("File: %016llx.%016llx\n", mini.name, mini.type);

		assert(mini.zero_1 == 0);
		assert(mini.zero_2 == 0);

		// Since the file IDs form a sequence of 1 upto the file count (though not in
		// order), we can use those as indexes into our file list.

		if (mini.fileId > filesList.size()) {
			filesList.resize(mini.fileId);
		}

		DslFile& fi = filesList.at(mini.fileId - 1);

		fi.name = mini.name;
		fi.type = mini.type;
		fi.fileId = mini.fileId;

		// Look up the language idstring, if applicable
		fi.rawLangId = mini.langId;
		if (mini.langId == 0)
			fi.langId = 0;
		else if (languages.count(mini.langId))
			fi.langId = languages[mini.langId];
		else
			fi.langId = 0x11df684c9591b7e0; // 'unknown' - is in the hashlist, so you'll be able to find it

		// If it's a repeated file, the language must be different
		const auto& prev = files.find(fi.Key());
		if (prev != files.end())
		{
			assert(prev->second->langId != fi.langId);
			fi.next = prev->second;
		}

		files[fi.Key()] = &fi;
	}

	//printf("File count: %ld\n", files.size());

	// Load each of the bundle headers
	std::string suffix = "_h.bundle";
	std::string prefix = "all_";
	for (const std::string& name : raidhook::Util::GetDirectoryContents("assets"))
	{
		if (name.length() <= suffix.size())
			continue;
		if (name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0)
			continue;
		if (name == "all_h.bundle")
			continue; // all_h handling later

		bool package = true;
		if (name.compare(0, prefix.size(), prefix) == 0)
			package = false;

		std::string headerPath = "assets/" + name;

		// Find the headerPath to the data file - chop out the '_h' bit
		std::string dataPath = headerPath;
		dataPath.erase(dataPath.end() - 9, dataPath.end() - 7);

		if (package)
			loadPackageHeader(headerPath, dataPath, filesList);
		else
			loadBundleHeader(headerPath, dataPath, filesList);
	}

	// We're done loading, print out how long it took and how many files it's tracking (to estimate memory usage)
	uint64_t end_time = monotonicTimeMicros();

	char buff[1024];
	memset(buff, 0, sizeof(buff));
	snprintf(buff, sizeof(buff) - 1, "Finished loading DB info: %zd files in %d ms", filesList.size(),
	         (int)(end_time - start_time) / 1000);
	RAIDHOOK_LOG_LOG(buff);
}

static void loadPackageHeader(std::string headerPath, std::string dataPath, FileList files)
{
	// Memory leak, not an issue since it's a small amount and the DB doesn't get unloaded anyway
	auto* bundle = new DieselBundle();
	bundle->headerPath = headerPath;
	bundle->path = dataPath;

	std::ifstream in;
	in.exceptions(std::ios::failbit | std::ios::badbit);
	in.open(bundle->headerPath, std::ios::binary);

	// Skip an int, the length of the header
	in.seekg(4, std::ios::cur);

	// Files
	struct FilePos
	{
		int32_t fileId;
		int32_t offset;
	};
	static_assert(sizeof(FilePos) == 8); // Same on 32 and 64 bit
	std::vector<FilePos> positions = loadVector<FilePos>(in, 4);

	DslFile* prev = nullptr;
	for (const FilePos& fp : positions)
	{
		DslFile* fi = &files.at(fp.fileId - 1);

		fi->bundle = bundle;
		fi->offset = fp.offset;
		if (prev != nullptr)
		{
			prev->length = fi->offset - prev->offset;
		}

		prev = fi;
	}

	// TODO set a length for the last file
}

static void loadBundleHeader(std::string headerPath, std::string dataPath, FileList files)
{
	std::ifstream in;
	in.exceptions(std::ios::failbit | std::ios::badbit);
	in.open(headerPath, std::ios::binary);

	// Skip an int, the length of the header
	in.seekg(4, std::ios::cur);

	struct BundleInfo
	{
		intptr_t id;
		dsl_Vector vec;
		intptr_t zero;
		intptr_t one;
	};

	static_assert(sizeof(BundleInfo) == 48);

	struct ItemInfo
	{
		uint32_t fileId;
		uint32_t offset;
		uint32_t length;
	};
	static_assert(sizeof(ItemInfo) == 12); // True on 32/64 bit

	BundleInfo bundle = {0};
	in.read((char*)&bundle, sizeof(bundle));

	assert(bundle.zero == 0);
	assert(bundle.one == 1);

	// Memory leak, not an issue since it's a small amount and the DB doesn't get unloaded anyway
	DieselBundle* dieselBundle = new DieselBundle();
	dieselBundle->headerPath = headerPath;
	dieselBundle->path = dataPath;

	for (ItemInfo item : loadVector<ItemInfo>(in, 4, bundle.vec))
	{
		DslFile* fi = &files.at(item.fileId - 1);
		fi->bundle = dieselBundle;
		fi->offset = item.offset;
		fi->length = item.length;
	}
}

DslFile* DieselDB::Find(idstring name, idstring ext)
{
	auto res = files.find(std::pair<idstring, idstring>(name, ext));

	// Not found?
	if (res == files.end())
	{
		return nullptr;
	}

	return res->second;
}

BLTAbstractDataStore* DieselDB::Open(DieselBundle* bundle)
{
	// Ideally we'd cache these to avoid opening files all the time, but this is
	// pretty much what the Linux version of PD2 seems to do internally. The
	// problem here is that the data store is reference counted by dsl::Archive, so
	// caching it will be somewhat difficult.
	// TODO: there is no raid linux binary tho, what now?
	BLTFileDataStore* fds = BLTFileDataStore::Open(bundle->path);
	return fds;
}
