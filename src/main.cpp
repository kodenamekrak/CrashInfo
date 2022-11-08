#include "main.hpp"
#include "SettingsViewController.hpp"
#include "Utils/Utils.hpp"
#include "ModConfig.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"

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
    
    auto crashes = Utils::GetCrashesFromUser(userId);

    string crashId = crashes[0];
    //crashId = "BsqG"; // Testing

    if (getModConfig().LastCrash.GetValue() == "")
        getModConfig().LastCrash.SetValue(crashId);

    getLogger().info("Latest crash: %s", crashId.c_str());

    // if (crashId == getModConfig().LastCrash.GetValue() || !getModConfig().ShowPopup.GetValue())
    //   return;

    shouldShowPopup = true;

    culprits = Utils::GetCulpritsFromId(crashId);

    for (auto val : culprits)
        getLogger().info("aaa %s", val.c_str());
}

MAKE_AUTO_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation && shouldShowPopup)
    {
        auto modal = BeatSaberUI::CreateModal(self->get_transform(), {50, 60}, nullptr);
        auto modalContainer = BeatSaberUI::CreateScrollableModalContainer(modal);

        for (auto val : culprits)
        {
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
    static Logger *logger = new Logger(modInfo, LoggerOptions(false, true));
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
    QuestUI::Register::RegisterMainMenuModSettingsViewController<crashinfo::CrashListViewController *>(modInfo, "CrashInfo");

    getModConfig().Init(modInfo);

    getLogger().info("Installing hooks...");
    Hooks::InstallHooks(getLogger());
    getLogger().info("Installed all hooks!");

    LoadCrashes();
}