#pragma once

#include "Button.h"
#include "Menu.h"

#include <string>
#include <vector>
#include <functional>

extern bool g_talk_auto_play;
extern int g_talk_auto_play_delay_ms;

// 战斗加速全局状态
extern int g_battle_speed;      // 0=0.5x, 1=1x, 2=2x, 3=4x
extern int g_battle_auto;       // 0=关闭, 1=自动战斗

// BattleScene 需要访问这些全局变量
extern int g_battle_auto;   // 自动战斗（BattleScene.cpp 会 extern 它）

class UIGameAssist : public Menu
{
public:
    UIGameAssist();
    ~UIGameAssist() override;

    void dealEvent(EngineEvent& e) override;
    void draw() override;
    void onPressedOK() override;
    void onPressedCancel() override
    {
        if (show_exit_popup_)
        {
            show_exit_popup_ = false;
            return;
        }
        if (has_unsaved_changes_)
        {
            show_exit_popup_ = true;
            exit_popup_selection_ = 0;
            return;
        }
        exitWithResult(-1);
    }

    // 供游戏循环调用：根据开关状态自动修正
    static void applyActiveCheats();

private:
    struct AssistItem
    {
        std::string label;
        std::string type;        // "number" | "toggle"
        int current_value = 0;
        int min_value = 0;
        int max_value = 99999;
        int step = 1;
        std::string ini_section; // kysmod.ini 中的 section
        std::string ini_key;     // kysmod.ini 中的 key
        bool modified = false;
        std::vector<std::string> value_labels;  // 非空时按 label 显示，而非数字
        std::shared_ptr<Button> row;
    };

    std::vector<AssistItem> items_;
    std::vector<std::shared_ptr<Button>> buttons_left_;
    std::vector<std::shared_ptr<Button>> buttons_right_;
    std::shared_ptr<Button> button_confirm_;
    std::shared_ptr<Button> button_cancel_;

    // 退出弹窗状态
    bool has_unsaved_changes_ = false;
    bool show_exit_popup_ = false;
    int exit_popup_selection_ = 0;  // 0=保存, 1=不保存

    void addItem(const std::string& label, const std::string& type,
                 int current, int min_v, int max_v, int step,
                 const std::string& ini_section, const std::string& ini_key,
                 const std::vector<std::string>& value_labels = {});
    void loadCurrentValues();
    void saveToIni();
    void changeOption(int index, int delta);
    bool isItemActive() const;

    static constexpr int ROW_WIDTH = 280;
    static constexpr int ROW_HEIGHT = 28;
    static constexpr int ROW_GAP = 30;
    static constexpr int ARROW_LEFT_OFFSET = 140;
    static constexpr int VALUE_OFFSET = 162;
    static constexpr int ARROW_RIGHT_OFFSET = 250;
    static constexpr int BUTTON_Y_OFFSET = 520;
};
