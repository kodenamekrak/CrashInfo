#include "main.hpp"
#include "SettingsViewController.hpp"
#include "Utils/WebUtils.hpp"
#include "ModConfig.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <string>

DEFINE_CONFIG(ModConfig);

using namespace GlobalNamespace;
using namespace std;
using namespace QuestUI;

bool shouldShowPopup;
const string crUrl = "https://analyzer.questmodding.com/api/crashes";
vector<string> culprits;
const vector<string> coreMods = {"libcustom-types.so", "libpinkcore.so", "libplaylistcore.so",
                                 "libplaylistmanager.so", "libcodegen.so", "libdatakeeper.so",
                                 "libmod-list.so", "libquestui.so", "libsongdownloader.so",
                                 "libsongloader.so"};

void LoadCrashes()
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
    getLogger().info("Content: %s", content.c_str());
    rapidjson::Document document;
    document.Parse(content.c_str());

    const string userId = document["UserId"].GetString();
    const string userSpec = "?userId=";
    getLogger().info("%s", userId.c_str());

    string response = Utils::RequestURL(crUrl + userSpec + userId);

    rapidjson::Document crashes;
    crashes.Parse(response);

    string crashId = crashes[0]["crashId"].GetString();
    crashId = "BsqG"; // Testing

    if (getModConfig().LastCrash.GetValue() == "")
        getModConfig().LastCrash.SetValue(crashId);
    getLogger().info("Latest crash: %s", crashId.c_str());

    // if (crashId == getModConfig().LastCrash.GetValue() || !getModConfig().ShowPopup.GetValue())
    //   return;

    shouldShowPopup = true;

    response = Utils::RequestURL(crUrl + "/" + crashId);
    rapidjson::Document crash;
    crash.Parse(response);
    
    string stacktrace = crash["stacktrace"].GetString();

    getLogger().info("Regexing crash");

    regex regexp("#[0-9]+ pc [0-9a-z]+  /data/data/com.beatgames.beatsaber/files/lib[a-zA-Z0-9_.-]+.so");
    regex libreg("lib[a-zA-Z0-9_.-]+.so");

    sregex_iterator iter(stacktrace.begin(), stacktrace.end(), regexp);
    sregex_iterator end;

    while (iter != end)
    {
        for (unsigned i = 0; i < iter->size(); ++i)
        {
            smatch m;
            line = (*iter)[i].str();
            regex_search(line, m, libreg);
            culprits.push_back(m[0].str());
        }
        ++iter;
    }
    std::sort(culprits.begin(), culprits.end());
    culprits.erase(std::unique(culprits.begin(), culprits.end()), culprits.end());

    for (auto val : culprits)
        getLogger().info("aaa %s", val.c_str());
}

MAKE_AUTO_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation)
    {
        auto modal = BeatSaberUI::CreateModal(self->get_transform(), {50, 60}, nullptr);
        auto modalContainer = BeatSaberUI::CreateScrollableModalContainer(modal);

        for (auto val : culprits)
        {
            if (std::find(coreMods.begin(), coreMods.end(), val) != coreMods.end())
                val += " (Core mod)";

            BeatSaberUI::CreateText(modalContainer->get_transform(), val.c_str());
        }

        modal->Show(true, false, nullptr);
    }
}

static ModInfo modInfo;

Configuration &getConfig()
{
    static Configuration config(modInfo);
    config.Load();
    return config;
}

Logger &getLogger()
{
    static Logger *logger = new Logger(modInfo);
    return *logger;
}

extern "C" void setup(ModInfo &info)
{
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Load();
    getLogger().info("Completed setup!");
}

extern "C" void load()
{
    if (!Modloader::requireMod("CrashReporter"))
    {
        getLogger().info("CrashReporter not found, mod is not installing");
        return;
    }
    else
        getLogger().info("CrashReporter found");

    il2cpp_functions::Init();

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController<crashinfo::SettingsViewController *>(modInfo, "CrashInfo");

    getModConfig().Init(modInfo);

    getLogger().info("Installing hooks...");
    Hooks::InstallHooks(getLogger());
    getLogger().info("Installed all hooks!");

    LoadCrashes();
}