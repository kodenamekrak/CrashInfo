#include "main.hpp"
#include "CrashListViewController.hpp"
#include "Utils/Utils.hpp"
#include "ModConfig.hpp"
#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "Utils/WebUtils.hpp"

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
    getLogger().info("Loading crashes");
    string userId = Utils::GetUserId();
    Utils::GetCrashesFromUser();

    string url = crUrl + "?userId=" + userId;
    WebUtils::GetAsync(url, [&](long code, string response)
    {
        rapidjson::Document doc;
        doc.Parse(response.c_str());
        string i = doc[0]["crashId"].GetString();
        string last = getModConfig().LastCrash.GetValue();
        getModConfig().LastCrash.SetValue(i);
        if(!getModConfig().ShowPopup.GetValue() || last == "" || last == i)
            return;

        string re = WebUtils::Get(crUrl + "/" + i, 10);
        doc.Parse(re.c_str());
        culprits = Utils::GetCulprits(doc["stacktrace"].GetString());

        shouldShowPopup = true;
    });

}

MAKE_AUTO_HOOK_MATCH(MainMenuViewController_DidActivate, &MainMenuViewController::DidActivate, void, MainMenuViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    MainMenuViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    // Not done yet
    if (firstActivation && shouldShowPopup)
    {
        if(culprits.empty())
            return;
    
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

    il2cpp_functions::Init();

    QuestUI::Init();
    QuestUI::Register::RegisterAllModSettingsViewController<crashinfo::CrashInfoListViewController *>(modInfo, "CrashInfo");

    getModConfig().Init(modInfo);

    getLogger().info("Installing hooks...");
    Hooks::InstallHooks(getLogger());
    getLogger().info("Installed all hooks!");

    LoadCrashes();
}