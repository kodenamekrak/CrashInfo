#include "Utils/Utils.hpp"
#include "libcurl/shared/curl.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "ModConfig.hpp"
#include "Utils/WebUtils.hpp"
#include "CrashListViewController.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/BeatSaberUI.hpp"

#include <fstream>
#include <regex>
#include <thread>

const string crUrl = "https://analyzer.questmodding.com/api/crashes";
string user;
vector<string> crashIds;
vector<string> culps;
const vector<string> coreMods = {"libcustom-types.so", "libpinkcore.so", "libplaylistcore.so",
                                 "libplaylistmanager.so", "libcodegen.so", "libdatakeeper.so",
                                 "libmod-list.so", "libquestui.so", "libsongdownloader.so",
                                 "libsongloader.so", "libbsml.so"};

namespace Utils
{
    string GetUserId()
    {
        string content;
        string line;
        fstream myfile;

        myfile.open("/sdcard/moddata/com.beatgames.beatsaber/configs/crashreporter.json");
        if (myfile.is_open())
        {
            while (getline(myfile, line))
                content += line;
        }
        myfile.close();
        rapidjson::Document document;
        document.Parse(content.c_str());

        user = document["UserId"].GetString();
        getLogger().info("User: %s", user.c_str());
        return user;
    }

    vector<string> GetCulprits(string stacktrace, bool createModal)
    {
        culps.clear();

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
                string lib = m[0].str();
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
        // for(auto val : culps)
        //     getLogger().info("lib: %s", val.c_str());

        if(createModal)
            CreateCrashModal(culps);
        
        return culps;
    }

    vector<string> GetCrashesFromUser()
    {
        string url = crUrl + "?userId=" + user;
        WebUtils::GetAsync(url, [&](long code, string response)
        {
            rapidjson::Document crashes;
            crashes.Parse(response.c_str());
            
            crashIds.clear();
            for(int i = 0; i < crashes.Size(); i++)
            {
                string id = crashes[i]["crashId"].GetString();
                crashIds.push_back(id);
            }
            getLogger().info("Crash ids: %lu", crashIds.size());
        });
        return crashIds;
    }
}