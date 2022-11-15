#include "main.hpp"
#include "CrashListViewController.hpp"
#include "Utils/Utils.hpp"
#include "ModConfig.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UnityEngine/Transform.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "Utils/WebUtils.hpp"
#include "TMPro/TextAlignmentOptions.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

DEFINE_TYPE(crashinfo, CrashInfoListViewController);

using namespace QuestUI;
using namespace std;

UnityEngine::Transform *settingsContainerTransform;

void CreateCrashModal(vector<string> culprits)
{
    auto modal = QuestUI::BeatSaberUI::CreateModal(settingsContainerTransform, {60, 70}, nullptr);
    auto modalContainer = QuestUI::BeatSaberUI::CreateScrollableModalContainer(modal);

    auto texts = modal->GetComponentsInChildren<HMUI::CurvedTextMeshPro*>();
    for (auto val : culprits)
    {
        QuestUI::BeatSaberUI::CreateText(modalContainer->get_transform(), val.c_str());
    }
    modal->Show(true, false, nullptr);
}

void crashinfo::CrashInfoListViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    if(firstActivation)
    {
        auto settingsContainer = BeatSaberUI::CreateScrollableSettingsContainer(get_transform());
        settingsContainerTransform = settingsContainer->get_transform();

        AddConfigValueToggle(settingsContainerTransform, getModConfig().ShowPopup);

        auto crashes = Utils::GetCrashesFromUser();

        if(crashes.empty())
        {
            BeatSaberUI::CreateText(settingsContainerTransform, "No crashes found");
            return;
        }

        for(int i = 0; i < crashes.size(); i++)
        {
            if(i == 6)
                break;
            string text = to_string(i + 1) + " - " + crashes[i];
            QuestUI::ClickableText* t = BeatSaberUI::CreateClickableText(settingsContainerTransform, text.c_str(), {0, 0}, [&, crashes, i]
            {
                string url = "https://analyzer.questmodding.com/api/crashes/" + crashes[i];
                WebUtils::GetAsync(url, [&](long code, string response)
                {
                    rapidjson::Document doc;
                    doc.Parse(response.c_str());
                    string stacktrace = doc["stacktrace"].GetString();
                    QuestUI::MainThreadScheduler::Schedule([stacktrace]()
                    {
                        CreateCrashModal(Utils::GetCulprits(stacktrace));
                    });
                });
            });
            t->set_alignment(TMPro::TextAlignmentOptions::Center);
        }
    }
}