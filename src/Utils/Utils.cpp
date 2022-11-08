#include "Utils/Utils.hpp"
#include "libcurl/shared/curl.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "ModConfig.hpp"

#include <regex>

const string crUrl = "https://analyzer.questmodding.com/api/crashes";
string user;
vector<string> crashIds;
const vector<string> coreMods = {"libcustom-types.so", "libpinkcore.so", "libplaylistcore.so",
                                 "libplaylistmanager.so", "libcodegen.so", "libdatakeeper.so",
                                 "libmod-list.so", "libquestui.so", "libsongdownloader.so",
                                 "libsongloader.so"};

namespace Utils
{
    size_t writefunc(void *ptr, size_t size, size_t nmemb, string *s)
    {
        s->append(static_cast<char *>(ptr), size * nmemb);
        return size * nmemb;
    }

    string RequestURL(string url)
    {
        CURL *curl;
        CURLcode res;
        string s;

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

    vector<string> GetCulpritsFromId(string crashId)
    {
        vector<string> culps;

        string response = RequestURL(crUrl + "/" + crashId);
        rapidjson::Document crash;
        crash.Parse(response);

        string stacktrace = crash["stacktrace"].GetString();

        getLogger().info("Regexing crash");

        regex regexp("#[0-9]+ pc [0-9a-z]+  /data/data/com.beatgames.beatsaber/files/lib[a-zA-Z0-9_.-]+.so");
        regex libreg("lib[a-zA-Z0-9_.-]+.so");

        sregex_iterator iter(stacktrace.begin(), stacktrace.end(), regexp);
        sregex_iterator end;
        string line;

        while (iter != end)
        {
            for (unsigned i = 0; i < iter->size(); ++i)
            {
                smatch m;
                line = (*iter)[i].str();
                regex_search(line, m, libreg);
                culps.push_back(m[0].str());
            }
            ++iter;
        }

        std::sort(culps.begin(), culps.end());
        culps.erase(std::unique(culps.begin(), culps.end()), culps.end());

        for (int i = 0; i < culps.size(); i++)
        {
            if (std::find(coreMods.begin(), coreMods.end(), culps[i]) != coreMods.end())
                culps[i] += " (Core mod)";
        }

        return culps;
    }

    vector<string> GetCrashesFromUser(string userId, bool useOldIfExists)
    {
        if(userId != "")
            user = userId;

        if(useOldIfExists && !crashIds.empty())
            return crashIds;

        string response = Utils::RequestURL(crUrl + "?userId=" + user);

        rapidjson::Document crashes;
        crashes.Parse(response);

        crashIds.clear();
        for(int i = 0; i < crashes.Size(); i++)
        {
            string id = crashes[i]["crashId"].GetString();
            crashIds.push_back(id);
        }

        return crashIds;
    }
}