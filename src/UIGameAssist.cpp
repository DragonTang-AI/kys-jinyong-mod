#include "UIGameAssist.h"
#include "BattleScene.h"
#include "Event.h"
#include "Font.h"
#include "GameUtil.h"
#include "Save.h"
#include "TextureManager.h"
#include <algorithm>
#include <unordered_map>

namespace
{
    int clampValue(int value, int min_v, int max_v)
    {
        return std::max(min_v, std::min(value, max_v));
    }
}

// 对话自动播放全局状态（Talk.cpp 会读取这些）
bool g_talk_auto_play = false;
int g_talk_auto_play_delay_ms = 2000;   // 默认 2 秒翻一页

// 战斗加速全局状态（BattleScene.cpp 会读取这些）
int g_battle_speed = 1;   // 0=0.5x, 1=1x, 2=2x, 3=4x，默认 1x
int g_battle_auto = 0;    // 0=关闭, 1=自动战斗

UIGameAssist::UIGameAssist()
{
    // 银两 — 数值编辑
    addItem("銀兩", "number", 0, 0, 999999, 1000, "cheat", "money");
    // 体力全满 — 开关
    addItem("體力全滿", "toggle", 0, 0, 1, 1, "cheat", "physical_full");
    // HP/MP全满 — 开关
    addItem("HP/MP全滿", "toggle", 0, 0, 1, 1, "cheat", "hpmp_full");
    // 全队满级 — 开关
    addItem("全隊滿級", "toggle", 0, 0, 1, 1, "cheat", "level_full");
    // 属性全满 — 开关
    addItem("屬性全滿", "toggle", 0, 0, 1, 1, "cheat", "attr_full");
    // 品德 — 数值编辑
    addItem("品德", "number", 0, 0, 100, 10, "cheat", "morality");
    // 声望 — 数值编辑
    addItem("聲望", "number", 0, 0, 999, 10, "cheat", "fame");
    // 资质 — 数值编辑
    addItem("資質", "number", 0, 0, 100, 5, "cheat", "iq");
    // 武功全满 — 开关
    addItem("武功全滿", "toggle", 0, 0, 1, 1, "cheat", "magic_full");
    // 游戏速度 — 五档
    addItem("遊戲速度", "number", 1, 0, 4, 1, "cheat", "game_speed",
            {"0.5x", "1x", "2x", "4x", "8x"});
    // 全图开启 — 开关
    addItem("全圖開啟", "toggle", 0, 0, 1, 1, "cheat", "allmap_open", {});
    // 对话跳过 — 提示：按 S 键
    addItem("對話跳過 [S]", "toggle", 0, 0, 1, 1, "cheat", "talk_skip", {});
    // 自动播放 — 开关
    addItem("自動播放", "toggle", 0, 0, 1, 1, "cheat", "talk_auto_play", {});
    // 自动播放延迟 — 数值（毫秒）
    addItem("自動播放延遲(ms)", "number", 2000, 500, 10000, 500, "cheat", "talk_auto_delay");
    // 战斗速度 — 四档
    addItem("戰鬥速度", "number", 1, 0, 3, 1, "cheat", "battle_speed",
            {"0.5x", "1x", "2x", "4x"});
    // 自动战斗 — 开关
    addItem("自動戰鬥", "toggle", 0, 0, 1, 1, "cheat", "battle_auto", {});

    // 确认 / 取消 按钮（底部居中，左右排列）
    button_confirm_ = std::make_shared<Button>();
    button_confirm_->setFontSize(24);
    button_confirm_->setText("確認");
    addChild(button_confirm_, 200, BUTTON_Y_OFFSET);

    button_cancel_ = std::make_shared<Button>();
    button_cancel_->setFontSize(24);
    button_cancel_->setText("取消");
    addChild(button_cancel_, 320, BUTTON_Y_OFFSET);

    has_unsaved_changes_ = false;
    loadCurrentValues();
}

UIGameAssist::~UIGameAssist()
{
}

void UIGameAssist::addItem(const std::string& label, const std::string& type,
    int current, int min_v, int max_v, int step,
    const std::string& ini_section, const std::string& ini_key,
    const std::vector<std::string>& value_labels)
{
    AssistItem item;
    item.label = label;
    item.type = type;
    item.current_value = current;
    item.min_value = min_v;
    item.max_value = max_v;
    item.step = step;
    item.ini_section = ini_section;
    item.ini_key = ini_key;
    item.value_labels = value_labels;

    item.row = std::make_shared<Button>();
    item.row->setFontSize(24);
    item.row->setText(label);
    item.row->setHaveBox(false);
    item.row->setTextColor({ 48, 32, 16, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 });
    item.row->setSize(ROW_WIDTH, ROW_HEIGHT);
    addChild(item.row, 0, int(items_.size()) * ROW_GAP);

    // 添加左右箭头按钮（与 UIConfig 风格一致）
    auto button_left = std::make_shared<Button>();
    button_left->setTexture("title", 104);
    item.row->addChild(button_left, ARROW_LEFT_OFFSET, 4);
    buttons_left_.push_back(button_left);

    auto button_right = std::make_shared<Button>();
    button_right->setTexture("title", 105);
    item.row->addChild(button_right, ARROW_RIGHT_OFFSET, 4);
    buttons_right_.push_back(button_right);

    items_.push_back(std::move(item));
}

void UIGameAssist::loadCurrentValues()
{
    auto* gu = GameUtil::getInstance();
    auto* save = Save::getInstance();

    // 所有项目统一从 ini 或游戏状态加载
    for (int i = 0; i < (int)items_.size(); i++)
    {
        auto& item = items_[i];
        if (item.ini_key == "money")
        {
            item.current_value = save->getMoneyCountInBag();
        }
        else if (item.ini_key == "morality")
        {
            auto* leader = save->getRole(save->Team[0]);
            item.current_value = leader ? leader->Morality : 50;
        }
        else if (item.ini_key == "fame")
        {
            auto* leader = save->getRole(save->Team[0]);
            item.current_value = leader ? leader->Fame : 0;
        }
        else if (item.ini_key == "iq")
        {
            auto* leader = save->getRole(save->Team[0]);
            item.current_value = leader ? leader->IQ : 50;
        }
        else
        {
            // toggle 和 number（游戏速度、战斗速度、延迟等）统一从 ini 读取
            item.current_value = gu->getInt(item.ini_section, item.ini_key, item.current_value);
        }
    }
}

void UIGameAssist::saveToIni()
{
    auto* gu = GameUtil::getInstance();
    for (auto& item : items_)
    {
        if (item.type == "toggle")
        {
            if (item.current_value == 0)
            {
                gu->setKey(item.ini_section, item.ini_key, "");
            }
            else
            {
                gu->setKey(item.ini_section, item.ini_key, "1");
            }
        }
        else if (item.type == "number" && item.modified)
        {
            gu->setKey(item.ini_section, item.ini_key, std::to_string(item.current_value));
        }
    }
    // 立即持久化到磁盘，避免应用异常退出时数据丢失
    gu->saveFile(GameUtil::PATH() + "config/kysmod.ini");
}

void UIGameAssist::changeOption(int index, int delta)
{
    if (index < 0 || index >= (int)items_.size()) { return; }
    auto& item = items_[index];
    item.modified = true;
    if (item.type == "number")
    {
        item.current_value += delta * item.step;
        item.current_value = clampValue(item.current_value, item.min_value, item.max_value);
    }
    else if (item.type == "toggle")
    {
        item.current_value = (item.current_value == 0) ? 1 : 0;
    }
    has_unsaved_changes_ = true;
}

bool UIGameAssist::isItemActive() const
{
    return active_child_ >= 0 && active_child_ < (int)items_.size();
}

void UIGameAssist::draw()
{
    for (int i = 0; i < (int)items_.size(); i++)
    {
        auto& item = items_[i];
        int row_x, row_y;
        item.row->getPosition(row_x, row_y);
        uint8_t alpha = item.row->getState() == NodeNormal ? 224 : 255;
        TextureManager::getInstance()->renderTexture("title", 126, row_x + 120, row_y, { { 255, 255, 255, 255 }, alpha }, ROW_WIDTH, ROW_HEIGHT);

        Color value_color = item.row->getState() == NodeNormal ? Color{ 48, 32, 16, 255 } : Color{ 255, 255, 255, 255 };

        std::string value_text;
        if (!item.value_labels.empty())
        {
            value_text = item.value_labels[item.current_value];
        }
        else if (item.type == "number")
        {
            value_text = std::to_string(item.current_value);
        }
        else if (item.type == "toggle")
        {
            value_text = item.current_value ? "開啟" : "關閉";
        }
        Font::getInstance()->draw(value_text, 24, row_x + VALUE_OFFSET, row_y, value_color, 255);
    }

    // 退出确认弹窗
    if (show_exit_popup_)
    {
        auto* engine = Engine::getInstance();
        int sw = engine->getUIWidth();
        int sh = engine->getUIHeight();

        // 半透明遮罩
        engine->fillColor({ 0, 0, 0, 180 }, 0, 0, sw, sh);

        // 弹窗主体：居中，280×180
        int box_w = 280;
        int box_h = 160;
        int box_x = (sw - box_w) / 2;
        int box_y = (sh - box_h) / 2;
        // 黑色背景 + 白色边框
        engine->fillColor({ 10, 10, 10, 230 }, box_x, box_y, box_w, box_h);
        engine->fillColor({ 255, 220, 100, 255 }, box_x, box_y, box_w, 2);       // 上边
        engine->fillColor({ 255, 220, 100, 255 }, box_x, box_y + box_h - 2, box_w, 2); // 下边
        engine->fillColor({ 255, 220, 100, 255 }, box_x, box_y, 2, box_h);        // 左边
        engine->fillColor({ 255, 220, 100, 255 }, box_x + box_w - 2, box_y, 2, box_h); // 右边

        // 标题文字
        Font::getInstance()->draw("是否保存修改？", 22, box_x, box_y + 15, { 255, 220, 100, 255 });

        // 两个选项
        std::string opts[2] = { "保存", "不保存" };
        int opt_start_x = box_x + 30;
        int opt_y = box_y + 65;
        for (int i = 0; i < 2; i++)
        {
            int opt_x = opt_start_x + i * 115;
            Color bg_color = (exit_popup_selection_ == i) ? Color{ 100, 60, 20, 200 } : Color{ 40, 40, 40, 180 };
            engine->fillColor(bg_color, opt_x, opt_y, 100, 36);
            Color text_color = (exit_popup_selection_ == i) ? Color{ 255, 220, 100, 255 } : Color{ 180, 180, 180, 255 };
            Font::getInstance()->draw(opts[i], 22, opt_x + 15, opt_y + 6, text_color);
        }

        // 底部提示
        Font::getInstance()->draw("← → 切换   Enter 確定   ESC 取消", 16,
                                  box_x, box_y + box_h - 28, { 160, 160, 160, 200 });
    }
}

void UIGameAssist::dealEvent(EngineEvent& e)
{
    // 弹窗内：左右切换选项，回车/ESC/取消键处理
    if (show_exit_popup_)
    {
        if (e.type == EVENT_KEY_DOWN)
        {
            if (e.key.key == K_LEFT || e.key.key == K_RIGHT)
            {
                exit_popup_selection_ = (exit_popup_selection_ == 0) ? 1 : 0;
            }
            else if (e.key.key == K_ESCAPE)
            {
                show_exit_popup_ = false;
            }
            else if (e.key.key == K_RETURN)
            {
                if (exit_popup_selection_ == 0)
                {
                    saveToIni();
                }
                has_unsaved_changes_ = false;
                for (auto& item : items_)
                    item.modified = false;
                show_exit_popup_ = false;
                exitWithResult(-1);
            }
        }
        return;
    }

    static int first_press = 0;
    if (e.type == EVENT_KEY_DOWN && isItemActive() && (e.key.key == K_LEFT || e.key.key == K_RIGHT))
    {
        if (first_press == 0 || first_press > 5)
        {
            changeOption(active_child_, e.key.key == K_LEFT ? -1 : 1);
        }
        first_press++;
    }
    else
    {
        Menu::dealEvent(e);
    }

    if (e.type == EVENT_KEY_UP && (e.key.key == K_LEFT || e.key.key == K_RIGHT))
    {
        first_press = 0;
    }
}

void UIGameAssist::onPressedOK()
{
    // 确认按钮：保存配置并关闭
    if (button_confirm_->getState() == NodePress)
    {
        saveToIni();
        has_unsaved_changes_ = false;
        for (auto& item : items_)
            item.modified = false;
        exitWithResult(0);
        return;
    }
    // 取消按钮
    if (button_cancel_->getState() == NodePress)
    {
        if (has_unsaved_changes_)
        {
            show_exit_popup_ = true;
            exit_popup_selection_ = 0;
            return;
        }
        exitWithResult(-1);
        return;
    }

    // 处理左右箭头按钮点击（与 UIConfig 风格一致）
    for (int i = 0; i < (int)items_.size(); i++)
    {
        if (buttons_left_[i]->getState() == NodePress)
        {
            changeOption(i, -1);
            return;
        }
        if (buttons_right_[i]->getState() == NodePress)
        {
            changeOption(i, 1);
            return;
        }
    }

    checkActiveToResult();
    if (result_ >= 0 && result_ < (int)items_.size())
    {
        changeOption(result_, 1);
        result_ = -1;
    }
}

// ====== 静态方法：供游戏循环调用 ======

static int s_prev_allmap_open = -1;
static std::unordered_map<int, int> s_allmap_backup;

void UIGameAssist::applyActiveCheats()
{
    auto* gu = GameUtil::getInstance();
    auto* save = Save::getInstance();
    if (!save) { return; }

    // 银两
    int target_money = gu->getInt("cheat", "money", -1);
    if (target_money >= 0)
    {
        int diff = target_money - save->getMoneyCountInBag();
        if (diff != 0)
        {
            Event::getInstance()->addItemWithoutHint(Item::MoneyItemID, diff);
        }
    }

    // 遍历队伍
    for (int i = 0; i < TEAMMATE_COUNT; i++)
    {
        if (save->Team[i] < 0) { continue; }
        auto* r = save->getRole(save->Team[i]);
        if (!r) { continue; }

        // 体力全满
        if (gu->getInt("cheat", "physical_full", 0))
        {
            r->PhysicalPower = save->MaxPhysicalPower;
        }
        // HP/MP全满
        if (gu->getInt("cheat", "hpmp_full", 0))
        {
            r->HP = r->MaxHP;
            r->MP = r->MaxMP;
            r->Hurt = 0;
            r->Poison = 0;
        }
        // 全队满级
        if (gu->getInt("cheat", "level_full", 0))
        {
            r->Level = save->MaxLevel;
            r->Exp = save->MaxExp;
            r->MaxHP = save->MaxHP;
            r->MaxMP = save->MaxMP;
        }
        // 属性全满
        if (gu->getInt("cheat", "attr_full", 0))
        {
            r->Attack = save->MaxAttack;
            r->Speed = save->MaxSpeed;
            r->Defence = save->MaxDefence;
            r->Medicine = save->MaxMedicine;
            r->UsePoison = save->MaxUsePoison;
            r->Detoxification = save->MaxDetoxification;
            r->AntiPoison = save->MaxAntiPoison;
            r->Fist = save->MaxFist;
            r->Sword = save->MaxSword;
            r->Knife = save->MaxKnife;
            r->Unusual = save->MaxUnusual;
            r->HiddenWeapon = save->MaxHiddenWeapon;
            r->Knowledge = save->MaxKnowledge;
            r->IQ = save->MaxIQ;
        }
        // 武功全满
        if (gu->getInt("cheat", "magic_full", 0))
        {
            for (int j = 0; j < ROLE_MAGIC_COUNT; j++)
            {
                if (r->MagicID[j] > 0)
                {
                    r->MagicLevel[j] = save->MaxLevel;
                }
            }
        }
    }

    // 全图开启（只在状态变化时执行一次）
    int allmap_open = gu->getInt("cheat", "allmap_open", 0);
    if (allmap_open != s_prev_allmap_open)
    {
        s_prev_allmap_open = allmap_open;
        if (allmap_open)
        {
            // 开启：先备份原始 EntranceCondition，再全图开启
            s_allmap_backup.clear();
            for (int i = 0; ; i++)
            {
                auto* sub = save->getSubMapInfo(i);
                if (!sub) { break; }
                s_allmap_backup[i] = sub->EntranceCondition;
                gu->setKey("cheat_allmap_backup", std::to_string(i),
                           std::to_string(sub->EntranceCondition));
            }
            Event::getInstance()->openAllSubMap();
        }
        else
        {
            // 关闭：从备份还原
            if (s_allmap_backup.empty())
            {
                // 内存无备份（如重启后），从 ini 恢复
                for (int i = 0; ; i++)
                {
                    auto* sub = save->getSubMapInfo(i);
                    if (!sub) { break; }
                    int orig = gu->getInt("cheat_allmap_backup",
                                            std::to_string(i), -1);
                    if (orig >= 0) { sub->EntranceCondition = orig; }
                }
            }
            else
            {
                for (auto& pair : s_allmap_backup)
                {
                    int id = pair.first;
                    int orig = pair.second;
                    auto* sub = save->getSubMapInfo(id);
                    if (sub) { sub->EntranceCondition = orig; }
                }
                s_allmap_backup.clear();
            }
        }
    }

    // 游戏速度
    static const double speed_table[] = {0.5, 1.0, 2.0, 4.0, 8.0};
    int speed_idx = gu->getInt("cheat", "game_speed", 1);
    if (speed_idx >= 0 && speed_idx <= 4)
    {
        Engine::setGameSpeed(speed_table[speed_idx]);
    }

    // 主角专属属性
    auto* leader = save->getRole(save->Team[0]);
    if (leader)
    {
        int v;
        v = gu->getInt("cheat", "morality", -1);
        if (v >= 0) { leader->Morality = v; }
        v = gu->getInt("cheat", "fame", -1);
        if (v >= 0) { leader->Fame = v; }
        v = gu->getInt("cheat", "iq", -1);
        if (v >= 0) { leader->IQ = v; }
    }

    // 对话自动播放
    g_talk_auto_play = gu->getInt("cheat", "talk_auto_play", 0) != 0;
    g_talk_auto_play_delay_ms = gu->getInt("cheat", "talk_auto_delay", 2000);
    if (g_talk_auto_play_delay_ms < 500) { g_talk_auto_play_delay_ms = 500; }

    // 战斗速度
    static const int battle_delay_table[] = {4, 2, 1, 1};  // animation_delay_ 对应值: 0=0.5x, 1=1x, 2=2x, 3=4x
    g_battle_speed = gu->getInt("cheat", "battle_speed", 1);
    if (g_battle_speed < 0) { g_battle_speed = 0; }
    if (g_battle_speed > 3) { g_battle_speed = 3; }
    BattleScene::setAnimationDelay(battle_delay_table[g_battle_speed]);

    // 自动战斗
    g_battle_auto = gu->getInt("cheat", "battle_auto", 0);
}
