#include "libcurl/shared/curl.h"
#include "main.hpp"

using namespace std;

namespace Utils
{
    size_t writefunc(void *ptr, size_t size, size_t nmemb, std::string *s)
    {
        s->append(static_cast<char *>(ptr), size * nmemb);
        return size * nmemb;
    }

    string RequestURL(string url)
    {
        CURL *curl;
        CURLcode res;
        std::string s;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (!curl)
            return nullptr;

        getLogger().info("Requesting URL: %s", url.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return s;
    }
}