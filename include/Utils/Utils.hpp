#include "main.hpp"

#include <vector>

using namespace std;

namespace Utils
{
    size_t writefunc(void *ptr, size_t size, size_t nmemb, string *s);

    string RequestURL(string url);

    vector<string> GetCulpritsFromId(string crashId);

    vector<string> GetCrashesFromUser(string userId = "", bool useOldIfExists = false);
}