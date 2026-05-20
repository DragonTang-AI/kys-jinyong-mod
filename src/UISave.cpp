#include "UISave.h"
#include "Event.h"
#include "Font.h"
#include "InputBox.h"
#include "MainScene.h"
#include "Menu.h"
#include "Save.h"
#include "SubScene.h"
#include "UI.h"
#include "filefunc.h"
#include <fstream>
#include <map>

// 存档元数据文件路径
static std::string getMetadataPath()
{
    return GameUtil::PATH() + "save/save_names.json";
}

// 加载存档名称
static std::map<int, std::string> loadSaveNames()
{
    std::map<int, std::string> names;
    auto path = getMetadataPath();
    if (!filefunc::fileExist(path))
    {
        return names;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        return names;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // 简单 JSON 解析（格式：{"1":"名称","2":"名称"}）
    size_t pos = 0;
    while ((pos = content.find('"', pos)) != std::string::npos)
    {
        size_t end = content.find('"', pos + 1);
        if (end == std::string::npos) break;
        int slot = 0;
        try { slot = std::stoi(content.substr(pos + 1, end - pos - 1)); } catch (...) { pos = end + 1; continue; }

        pos = content.find('"', end + 1);
        if (pos == std::string::npos) break;
        end = content.find('"', pos + 1);
        if (end == std::string::npos) break;
        names[slot] = content.substr(pos + 1, end - pos - 1);

        pos = end + 1;
    }

    return names;
}

// 保存存档名称
static void saveSaveNames(const std::map<int, std::string>& names)
{
    auto path = getMetadataPath();
    std::ofstream file(path);
    if (!file.is_open())
    {
        return;
    }

    file << "{";
    bool first = true;
    for (const auto& [slot, name] : names)
    {
        if (!first) file << ",";
        file << std::format("\"{}\":\"{}\"", slot, name);
        first = false;
    }
    file << "}";
    file.close();
}

// 全局存档名称缓存
static std::map<int, std::string> g_save_names;

// 获取存档显示名称
static std::string getSaveDisplayName(int i)
{
    if (g_save_names.count(i) && !g_save_names.at(i).empty())
    {
        return g_save_names.at(i);
    }
    return std::format("進度{:02}", i);
}

UISave::UISave()
{
    // 加载存档名称
    g_save_names = loadSaveNames();

    auto get_save_time = [](int i) -> std::string
    {
        auto filename = Save::getInstance()->getFilename(i, '\0');
        if (!filefunc::fileExist(filename))
        {
            filename = Save::getInstance()->getFilename(i, 'r');
        }
        auto str = filefunc::getFileTime(filename);
        if (str.empty())
        {
            str = "-------------------";
        }
        return str;
    };

    // 20 个手动存档位
    std::vector<std::string> strings;
    for (int i = 0; i <= MAX_SAVE_SLOT; i++)
    {
        auto display_name = getSaveDisplayName(i);
        auto str = std::format("{}  {}", display_name, get_save_time(i));
        strings.push_back(str);
    }
    // 自动档
    auto str = std::format("自動檔  {}", get_save_time(AUTO_SAVE_ID));
    strings.push_back(str);
    setStrings(strings);
    childs_[0]->setVisible(false);    // 屏蔽进度0
    forceActiveChild(1);
    arrange(0, 0, 0, 28);
}

UISave::~UISave()
{
}

void UISave::onEntrance()
{
    // 存档时屏蔽自动档
    if (mode_ == 1)
    {
        childs_.back()->setVisible(false);
    }
}

void UISave::onPressedOK()
{
    checkActiveToResult();
    if (result_ >= 0)
    {
        if (mode_ == 0)
        {
            setExit(load(result_));
        }
        if (mode_ == 1)
        {
            Event::getInstance()->arrangeBag();    // 存档时会整理物品背包

            // 存档命名：直接弹 InputBox，不命名则用默认名
            int slot = result_;
            std::string current_name = g_save_names.count(slot) ? g_save_names[slot] : "";

            auto input = std::make_shared<InputBox>();
            input->setTitle(std::format("存檔命名（留空使用默認名）"));
            input->setFontSize(24);
            input->setText(current_name);
            int w = Engine::getInstance()->getUIWidth();
            int h = Engine::getInstance()->getUIHeight();
            input->runAtPosition(w / 2 - 150, h / 2 - 60);

            std::string new_name = input->getText();
            // 去除前后空格
            size_t start = new_name.find_first_not_of(" \t\n\r");
            size_t end = new_name.find_last_not_of(" \t\n\r");
            if (start != std::string::npos && end != std::string::npos)
            {
                new_name = new_name.substr(start, end - start + 1);
            }
            else
            {
                new_name = "";
            }

            if (!new_name.empty())
            {
                g_save_names[slot] = new_name;
                saveSaveNames(g_save_names);
            }

            // 执行存档
            save(slot);
            setExit(true);
        }
    }
}

bool UISave::load(int r)
{
    auto sub_scene = getPointerFromRoot<SubScene>();    // 可以知道在不在子场景中
    auto save = Save::getInstance();
    auto main_scene = MainScene::getInstance();
    if (save->load(r))
    {
        main_scene->setManPosition(save->MainMapX, save->MainMapY);
        if (save->InSubMap >= 0)
        {
            if (sub_scene)
            {
                sub_scene->forceJumpSubScene(save->InSubMap, save->SubMapX, save->SubMapY);
            }
            else
            {
                main_scene->forceEnterSubScene(save->InSubMap, save->SubMapX, save->SubMapY);
            }
        }
        else
        {
            if (sub_scene)
            {
                sub_scene->forceExit();
            }
        }
        UI::getInstance()->setExit(true);
        return true;
    }
    else
    {
        auto text = std::make_shared<TextBox>();
        text->setText("存檔讀取失敗!");
        text->setFontSize(24);
        text->setTextColor({255, 0, 0});
        text->runAtPosition(Engine::getInstance()->getUIWidth() / 2 - 100, 620);
    }
    return false;
}

void UISave::save(int r)
{
    auto sub_scene = getPointerFromRoot<SubScene>();    // 可以知道在不在子场景中
    auto save = Save::getInstance();
    auto main_scene = MainScene::getInstance();
    main_scene->getManPosition(save->MainMapX, save->MainMapY);
    if (sub_scene)
    {
        sub_scene->getManPosition(save->SubMapX, save->SubMapY);
        save->InSubMap = sub_scene->getMapInfo()->ID;
    }
    else
    {
        save->InSubMap = -1;
    }
    save->save(r);
}
