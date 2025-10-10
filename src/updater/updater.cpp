#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <format>
#include <iostream>
#include <filesystem>

//#include "util/util.h"

static const char *DLL_UPDATE_FILE = "sblt_dll.zip";
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

int main(int argc, char *argv[])
{
    /* return codes:
        0: no update found/done
        1: update successful
        2: error while updating
    */
    if (argc < 2)
    {
        return 2;
    }
    
	// init curl
	curl_global_init(CURL_GLOBAL_ALL);
	// check which dll is used
	std::string DLL = "WSOCK32.dll";
	std::string DLL_old = "WSOCK32.dll.old";
	std::ifstream infile_iphlpapi("IPHLPAPI.dll");
	std::ostringstream datastream;
    std::string URL;
	if (infile_iphlpapi.good())
	{
		DLL = "IPHLPAPI.dll";
		DLL_old = "IPHLPAPI.dll.old";
	}

	// remove left over old dll
	if (std::filesystem::exists(DLL_old.c_str()))
	{
		std::filesystem::remove(DLL_old.c_str());
	}

	// check for updates

	// get remote version
	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // debug
	char errbuf[CURL_ERROR_SIZE];
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

	if (DLL == "IPHLPAPI.dll")
	{
		curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL_DLL_IPHLPAPI);
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL_DLL_WSOCK32);
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_stream);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &datastream);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		return 2;
	}

	std::string remote_version = datastream.str();

	// get local version
	std::string local_version = argv[1];

	printf("remote: %s\n", remote_version.c_str());
	printf("local: %s\n", local_version.c_str());

	// compare versions
	int lVerMaj = 0;
	int lVerMin = 0;
	int lVerPatch = 0;
	int lVerRev = 0;
	int rVerMaj = 0;
	int rVerMin = 0;
	int rVerPatch = 0;
	int rVerRev = 0;
	bool newer = false;

	sscanf(local_version.c_str(), "%d.%d.%d.%d", &lVerMaj, &lVerMin, &lVerPatch, &lVerRev);
	sscanf(remote_version.c_str(), "%d.%d.%d.%d", &rVerMaj, &rVerMin, &rVerPatch, &rVerRev);

	if (rVerMaj > lVerMaj)
	{
		newer = true;
	}
	if (rVerMin > lVerMin and newer == false)
	{
		newer = true;
	}
	if (rVerPatch > lVerPatch and newer == false)
	{
		newer = true;
	}
	if (rVerRev > lVerRev and newer == false)
	{
		newer = true;
	}

	// download new dll
	if (newer == true)
	{
		int result = MessageBox(NULL, "Do you want to update the RAID SuperBLT DLL?\nThis is recommended.", "SuperBLT DLL out of date", MB_YESNO);
		if (result == IDNO){
			curl_easy_cleanup(curl);
			return 0;
		}
		if (DLL == "IPHLPAPI.dll")
		{
			curl_easy_setopt(curl, CURLOPT_URL, DOWNLOAD_URL_DLL_IPHLPAPI);
            URL = DOWNLOAD_URL_DLL_IPHLPAPI;
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_URL, DOWNLOAD_URL_DLL_WSOCK32);
            URL = DOWNLOAD_URL_DLL_WSOCK32;
		}

		FILE *pagefile = NULL;
		errno_t err = fopen_s(&pagefile, DLL_UPDATE_FILE, "wb");
		if (err != 0)
		{
			/* cleanup curl stuff */
			curl_easy_cleanup(curl);
			printf("\nError opening output file %s - err %d\n", DLL_UPDATE_FILE, err);
			MessageBox(0, "An error occured.", "BLT Downloader", MB_OK);
			return 2;
		}

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
		CURLcode res2 = curl_easy_perform(curl);

		fclose(pagefile);

		if (res2 != CURLE_OK)
		{
			printf("\nError downloading SBLT DLL with error %d (URL=%s)\nERR: %s\n", res2, URL.c_str(), errbuf);
			MessageBox(0, "An error occured.", "BLT Downloader", MB_OK);
			return 2;
		}

		// move old dll
		try 
		{
			std::filesystem::rename(DLL.c_str(), DLL_old.c_str());
		}
		catch (std::filesystem::filesystem_error& e)
		{
			printf("%s\n", e.what());
			MessageBox(0, "An error occured.", "BLT Downloader", MB_OK);
			return 2;
		}

		// unpack new dll
		//raidhook::ExtractZIPArchive(DLL_UPDATE_FILE, ".");

		//clean up
		std::filesystem::remove(DLL_UPDATE_FILE);
		curl_easy_cleanup(curl);

		// tell user to restart game
		MessageBox(0, "SuperBLT DLL was updated successfully.\nPlease restart your game.", "SuperBLT DLL Updater", MB_OK);
		return 1;

	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl);
	return 0;
}