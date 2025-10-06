#include "http.h"
#include "util/util.h"
#include <curl/curl.h>
#include "platform.h"
#include <fstream>
#include <sstream>
#include <string>
#include <format>
#include <iostream>

#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

static const char *DOWNLOAD_URL = "https://api.modworkshop.net/mods/49758/download";
static const char *OUT_FILE_NAME = "blt_basemod_download.zip";

static const char *DOWNLOAD_URL_DLL_WSOCK32 = "https://api.modworkshop.net/mods/49746/download";
static const char *DOWNLOAD_URL_DLL_IPHLPAPI = "https://api.modworkshop.net/mods/49745/download";

static const char *VERSION_URL_DLL_WSOCK32 = "https://api.modworkshop.net/mods/49746/version";
static const char *VERSION_URL_DLL_IPHLPAPI = "https://api.modworkshop.net/mods/49745/version";

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

size_t write_data_stream(char *ptr, size_t size, size_t nmemb, void *userdata) {
    std::ostringstream *stream = (std::ostringstream*)userdata;
    size_t count = size * nmemb;
    stream->write(ptr, count);
    return count;
}

std::string GetDllVersion()
{
	HMODULE hModule;
	std::string ret = "0.0.0.0";
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(GetDllVersion), &hModule);
	char path[MAX_PATH + 1];
	size_t pathSize = GetModuleFileName(hModule, path, sizeof(path) - 1);
	path[pathSize] = '\0';

	DWORD verHandle = 0;
	UINT size = 0;
	LPBYTE lpBuffer = NULL;
	uint32_t verSize = GetFileVersionInfoSize(path, &verHandle);

	if (verSize == 0)
	{
		return "0.0.0.0";
	}

	std::string verData;
	verData.resize(verSize);

	if (!GetFileVersionInfo(path, verHandle, verSize, verData.data()))
	{
		return ret;
	}

	if (!VerQueryValue(verData.data(), "\\", (VOID FAR * FAR *)&lpBuffer, &size))
	{
		return ret;
	}

	if (size == 0)
	{
		return ret;
	}

	VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
	if (verInfo->dwSignature != 0xfeef04bd)
	{
		return ret;
	}

	ret = std::format("{}.{}.{}.{}",
										 (verInfo->dwFileVersionMS >> 16) & 0xFFFF,
										 (verInfo->dwFileVersionMS >> 0) & 0xFFFF,
										 (verInfo->dwFileVersionLS >> 16) & 0xFFFF,
										 (verInfo->dwFileVersionLS >> 0) & 0xFFFF);
	return ret;
}

void raidhook::download_blt()
{
	blt::platform::win32::OpenConsole();
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	printf("Downloading BLT...\n");

	// HTTP Manager shouldn't have started yet - do this ourselves
	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, DOWNLOAD_URL); // Set the URL to download
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // Enable the progress meter
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

	// Put our errors into a buffer
	char errbuf[CURL_ERROR_SIZE];
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

	// Open our ZIP file
	FILE *pagefile = NULL;
	errno_t err = fopen_s(&pagefile, OUT_FILE_NAME, "wb");
	if (err != 0)
	{
		/* cleanup curl stuff */
		curl_easy_cleanup(curl);

		SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
		printf("\nError opening output file %s - err %d\n", OUT_FILE_NAME, err);
		MessageBox(0, "An error occured.", "BLT Downloader", MB_OK);
		exit(0);
	}

	// Pass the data into the write_data function, along with the file handle.
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);

	// Run the download
	SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	CURLcode cerr = curl_easy_perform(curl);

	// Close the file
	fclose(pagefile);

	// Check for errors
	if (cerr != CURLE_OK)
	{
		SetConsoleTextAttribute(hStdout, FOREGROUND_RED | FOREGROUND_INTENSITY);

		printf("\nError downloading basemod with error %d (URL=%s)\nERR: %s\n", cerr, DOWNLOAD_URL, errbuf);
		MessageBox(0, "An error occured.", "BLT Downloader", MB_OK);
		exit(0);
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl);

	SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	printf("Download done. Installing...\n");

	CreateDirectory("mods", NULL);
	raidhook::ExtractZIPArchive(OUT_FILE_NAME, "mods");

	SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	printf("Cleaning up...\n");

	DeleteFile(OUT_FILE_NAME);

	SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("Installation complete. Please restart the game.\n");

	MessageBox(0, "The installation has finished.\nPlease restart the game.", "SuperBLT Downloader", MB_OK);
	exit(0);
}

void update_blt_dll()
{
	printf("update_blt_dll:start");
	printf("update_blt_dll:console");
	blt::platform::win32::OpenConsole();
	// init curl
	printf("update_blt_dll:curl_init");
	curl_global_init(CURL_GLOBAL_ALL);
	std::ostringstream stream;
	// check which dll is used
	printf("update_blt_dll:dll_check");
	std::string DLL = "WSOCK32.dll";
	std::ifstream infile_iphlpapi("IPHLPAPI.dll");
	if (infile_iphlpapi.good())
	{
		DLL = "IPHLPAPI.dll";
	}
	printf("DLL used: %s\n", DLL.c_str());
	// check for updates

	// get remote version
	printf("update_blt_dll:remote_version");
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // Enable the progress meter

	if (DLL == "IPHLPAPI.dll")
	{
		curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL_DLL_IPHLPAPI);
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL_DLL_WSOCK32);
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_stream);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		printf("error while downloading");
		curl_easy_cleanup(curl);
		exit(0);
	}

	std::string remote_version = stream.str();

	// get local version
	std::string local_version = GetDllVersion();

	printf("remote: %s\n", remote_version.c_str());
	printf("local: %s\n", local_version.c_str());
	

	/* cleanup curl stuff */
	curl_easy_cleanup(curl);
}