#include "main.hpp"
#include "SettingsViewController.hpp"
#include "Utils/Utils.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "UnityEngine/Transform.hpp"

DEFINE_TYPE(crashinfo, CrashListViewController);
DEFINE_TYPE(crashinfo, CrashInfoViewController);
DEFINE_TYPE(crashinfo, CrashInfoMenuViewController);

using namespace QuestUI;
using namespace std;

void CreateCrashModal(string id, UnityEngine::Transform *parent)
{
    getLogger().info("Creating modal");
    auto modal_ = BeatSaberUI::CreateModal(parent, {50, 60}, nullptr);
    auto modalContainer = BeatSaberUI::CreateScrollableModalContainer(modal_);

    //getLogger().info("Getting culprits for crash %s", id.c_str());
    auto culprits = Utils::GetCulpritsFromId(id);

    getLogger().info("Populating modal");
    for (auto val : culprits)
    {
        BeatSaberUI::CreateText(modalContainer->get_transform(), val.c_str());
    }

    getLogger().info("Showing modal");
    // for some reason crashes on showing modal, even if it has no children
    modal_->Show(true, false, nullptr);
}

void crashinfo::CrashListViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    if(firstActivation)
    {
        auto settingsContainerTransform = BeatSaberUI::CreateScrollableSettingsContainer(get_transform())->get_transform();
        auto ViewControllerTransform = get_transform();

        auto modal = BeatSaberUI::CreateModal(ViewControllerTransform, {50, 60}, nullptr);
        modal->Show(true, true, nullptr);
        auto crashes = Utils::GetCrashesFromUser("", true);

        for(int i = 0; i < crashes.size(); i++)
        {
            const string cri = crashes[i];
            getLogger().info("%s", cri.c_str());
            BeatSaberUI::CreateClickableText(settingsContainerTransform, crashes[i].c_str(), {0, 0}, [&]()
            {
                //getLogger().info("Text clicked %s", crashes[i].c_str());
                //CreateCrashModal("dXuq", settingsContainerTransform);
            });
        }
    }
}

void crashinfo::CrashInfoViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{}

void crashinfo::CrashInfoMenuViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{}

void crashinfo::CrashInfoMenuViewController::DidDeactivate(bool removedFromHierarchy, bool systemScreenDisabling)
{}