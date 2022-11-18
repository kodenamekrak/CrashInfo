#include "Utils/Utils.hpp"
#include "libcurl/shared/curl.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "ModConfig.hpp"
#include "Utils/WebUtils.hpp"

#include <fstream>
#include <regex>

const std::string crUrl = "https://analyzer.questmodding.com/api/crashes";
std::string user;
std::vector<std::string> crashIds;
std::vector<std::string> culps;
const std::vector<std::string> coreMods = {"libcustom-types.so", "libpinkcore.so", "libplaylistcore.so",
                                 "libplaylistmanager.so", "libcodegen.so", "libdatakeeper.so",
                                 "libmod-list.so", "libquestui.so", "libsongdownloader.so",
                                 "libsongloader.so", "libbsml.so"};

namespace Utils
{
    std::string GetUserId()
    {
        std::string content;
        std::string line;
        std::fstream myfile;

        myfile.open("/sdcard/moddata/com.beatgames.beatsaber/configs/crashreporter.json");
        if (myfile.is_open())
        {
            while (std::getline(myfile, line))
                content += line;
        }
        myfile.close();
        rapidjson::Document document;
        document.Parse(content.c_str());

        user = document["UserId"].GetString();
        getLogger().info("User: %s", user.c_str());
        return user;
    }

    std::vector<std::string> GetCulprits(std::string stacktrace)
    {
        culps.clear();

        std::regex regexp("#[0-9]+ pc [0-9a-z]+  /data/data/com.beatgames.beatsaber/files/lib[a-zA-Z0-9_.-]+.so");
        std::regex libreg("lib[a-zA-Z0-9_.-]+.so");

        std::sregex_iterator iter(stacktrace.begin(), stacktrace.end(), regexp);
        std::sregex_iterator end;
        std::string line;

        while (iter != end)
        {
            for (unsigned i = 0; i < iter->size(); ++i)
            {
                std::smatch m;
                line = (*iter)[i].str();
                std::regex_search(line, m, libreg);
                std::string lib = m[0].str();
                int index = lib.length();
                if (std::find(coreMods.begin(), coreMods.end(), lib) != coreMods.end())
                    lib += " (Core mod)";
                if(getModConfig().Simple.GetValue())
                {
                    lib.erase(0, 3);
                    lib.erase(index - 6, 3);
                }
                culps.push_back(lib);
            }
            ++iter;
        }

        std::sort(culps.begin(), culps.end());
        culps.erase(std::unique(culps.begin(), culps.end()), culps.end());

        if(culps.empty())
            culps.push_back("Could not find culprit");
        
        return culps;
    }

    std::vector<std::string> GetCrashesFromUser()
    {
        std::string url = crUrl + "?userId=" + user;
        std::string response = WebUtils::Get(url, 10);
        rapidjson::Document crashes;
        crashes.Parse(response.c_str());
        
        crashIds.clear();
        for(int i = 0; i < crashes.Size(); i++)
        {
            std::string id = crashes[i]["crashId"].GetString();
            crashIds.push_back(id);
        }
        getLogger().info("Crash ids: %lu", crashIds.size());

        return crashIds;
    }
}