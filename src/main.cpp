#include "main.hpp"
#include "SettingsViewController.hpp"
#include "Utils/WebUtils.hpp"
#include "ModConfig.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "json/json.h"
#include "modloader/shared/modloader.hpp"

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

void LoadCrashes()
{
    string content;
    string line;
    fstream myfile;
    Json::Value CrashReporterJson;
    Json::Reader reader;

    myfile.open("/sdcard/moddata/com.beatgames.beatsaber/configs/crashreporter.json");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
            content += line;
    }
    myfile.close();
    getLogger().info("Content: %s", content.c_str());

    reader.parse(content, CrashReporterJson);
    const string userId = CrashReporterJson["UserId"].asString();
    const string userSpec = "?userId=";

    string response = Utils::RequestURL(crUrl + userSpec + userId);

    Json::Value crashes;
    bool parsingSuccessful = reader.parse(response, crashes);
    if (!parsingSuccessful)
    {
        getLogger().info("Error parsing the crash list");
        return;
    }
    string crashId = crashes[0]["crashId"].asString();
    crashId = "BsqG"; // Testing

    if (getModConfig().LastCrash.GetValue() == "")
        getModConfig().LastCrash.SetValue(crashId);
    getLogger().info("Latest crash: %s", crashId.c_str());

    // if (crashId == getModConfig().LastCrash.GetValue() || !getModConfig().ShowPopup.GetValue())
    //   return;

    shouldShowPopup = true;

    response = Utils::RequestURL(crUrl + "/" + crashId);
    Json::Value crash;
    parsingSuccessful = reader.parse(response, crash);
    if (!parsingSuccessful)
    {
        getLogger().info("Error parsing the crash");
        return;
    }
    string stacktrace = crash["stacktrace"].asString();

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
    std::sort( culprits.begin(), culprits.end() );
    culprits.erase( std::unique( culprits.begin(), culprits.end() ), culprits.end() );

    for (auto val : culprits)
        getLogger().info("aaa %s", val.c_str());
}

MAKE_AUTO_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation)
    {
        if (shouldShowPopup)
        {
            auto modal = BeatSaberUI::CreateModal(self->get_transform(), {35, 60}, nullptr);
        }
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