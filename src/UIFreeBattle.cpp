#include "UIFreeBattle.h"
#include "Engine.h"
#include "TextureManager.h"
#include "BattleScene.h"
#include "BattleSceneHades.h"
#include "BattleSceneSekiro.h"
#include "BattleScenePaper.h"
#include "Event.h"
#include "../mlcc/PotConv.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

UIFreeBattle::UIFreeBattle()
{
    auto& roles = Save::getInstance()->getRoles();
    all_roles_.assign(roles.begin(), roles.end());
    battle_field_count_ = BattleMap::getInstance()->getBattleFieldCount();
    memset(friendly_ids_, -1, sizeof(friendly_ids_));
    memset(enemy_ids_, -1, sizeof(enemy_ids_));
    field_cursor_ = 0;
}

void UIFreeBattle::onEntrance()
{
    auto& roles = Save::getInstance()->getRoles();
    all_roles_.assign(roles.begin(), roles.end());
    battle_field_count_ = BattleMap::getInstance()->getBattleFieldCount();
    hint_timer_ = 0;
    search_text_.clear();
    search_active_ = false;
    battle_stats_.clear();
    battle_winner_ = -1;
    result_page_ = 0;
}

int UIFreeBattle::getRolePageCount() const
{
    auto filtered = const_cast<UIFreeBattle*>(this)->getFilteredRoles();
    int sz = (int)filtered.size();
    return (sz + getRolePageSize() - 1) / getRolePageSize();
}

int UIFreeBattle::getFieldPageCount() const
{
    return (battle_field_count_ + getFieldPageSize() - 1) / getFieldPageSize();
}

void UIFreeBattle::playSelectSound()
{
    Audio::getInstance()->playESound(0);
}

// ==================== DRAW ====================

void UIFreeBattle::draw()
{
    int sw = Engine::getInstance()->getWindowWidth();
    int sh = Engine::getInstance()->getWindowHeight();
    auto font = Font::getInstance();

    select_flash_timer_++;

    if (state_ == FreeBattleState::BattleResult)
    {
        drawBattleResult();
        return;
    }

    // 背景
    Engine::getInstance()->fillColor({ 0, 0, 0, 200 }, 0, 0, sw, sh);

    // 标题
    font->draw("自由對戰", 24, sw / 2 - 60, 8, { 255, 215, 0, 255 });

    // 状态标签
    std::string state_names[] = { "我方陣營", "敵方陣營", "選擇戰場", "準備開戰" };
    int tab_w = 90;
    int tab_start_x = sw / 2 - 2 * tab_w;
    for (int i = 0; i < 4; i++)
    {
        int tx = tab_start_x + i * tab_w;
        Color c = (int)state_ == i ? Color{ 255, 215, 0, 255 } : Color{ 180, 180, 180, 200 };
        font->draw(state_names[i], 16, tx, 36, c);
        if ((int)state_ == i)
        {
            Engine::getInstance()->fillColor({ 255, 215, 0, 255 }, tx, 54, tab_w - 10, 2);
        }
    }

    int content_y = 60;
    int content_h = sh - content_y - 30;

    switch (state_)
    {
    case FreeBattleState::SelectFriendly:
    case FreeBattleState::SelectEnemy:
    {
        int* selected_ids = (state_ == FreeBattleState::SelectFriendly) ? friendly_ids_ : enemy_ids_;
        int& count = (state_ == FreeBattleState::SelectFriendly) ? friendly_count_ : enemy_count_;
        int max_count = (state_ == FreeBattleState::SelectFriendly) ? TEAMMATE_COUNT : BATTLE_ENEMY_COUNT;
        Color team_color = (state_ == FreeBattleState::SelectFriendly) ? Color{ 100, 200, 255, 255 } : Color{ 255, 100, 100, 255 };
        std::string team_title = (state_ == FreeBattleState::SelectFriendly) ? "我方" : "敵方";

        // 计算布局
        int list_w = getListWidth();
        int detail_w = 380;
        int teamlist_w = 220;
        int panel_h = content_h;
        int left_w = sw - list_w - detail_w - 30;

        // 中间：角色详情面板
        auto filtered = getFilteredRoles();
        Role* selected = nullptr;
        if (role_cursor_ >= 0 && role_cursor_ < (int)filtered.size())
            selected = filtered[role_cursor_];
        drawRoleDetail(10 + left_w + 10, content_y, detail_w, panel_h, selected, team_color);

        // 左侧：已选阵容实时列表
        drawSelectedTeamCompact(10, content_y, left_w, panel_h, friendly_ids_, friendly_count_, TEAMMATE_COUNT, "我方", { 100, 200, 255, 255 });
        drawSelectedTeamCompact(10, content_y + panel_h / 2 + 10, left_w, panel_h / 2 - 10, enemy_ids_, enemy_count_, BATTLE_ENEMY_COUNT, "敵方", { 255, 100, 100, 255 });

        // 右侧：角色列表
        drawRoleList(sw - list_w - 10, content_y, list_w, content_h, filtered);

        // 分页
        drawPagination(sw - list_w - 10, sh - 28, role_page_, getRolePageCount());

        // 搜索提示
        if (search_active_)
        {
            font->draw("搜索: " + search_text_ + "_", 16, sw - list_w - 10, sh - 50, { 200, 200, 200, 255 });
        }

        // 操作提示
        font->draw("上下選人 | Enter加入/移除 | Del刪除最後 | 左右換頁 | Tab切換", 12, 10, sh - 18, { 150, 150, 150, 200 });
        break;
    }
    case FreeBattleState::SelectField:
    {
        // 左侧：阵容预览（居中）
        int left_w = sw - getListWidth() - 30;
        drawSelectedTeam(10, content_y, left_w, friendly_ids_, friendly_count_, TEAMMATE_COUNT, "我方", { 100, 200, 255, 255 });
        drawSelectedTeam(10, content_y + 220, left_w, enemy_ids_, enemy_count_, BATTLE_ENEMY_COUNT, "敵方", { 255, 100, 100, 255 });

        // 右侧：战场列表
        drawBattleFieldList(sw - getListWidth() - 10, content_y, getListWidth(), content_h);

        // 分页
        drawPagination(sw - getListWidth() - 10, sh - 28, field_page_, getFieldPageCount());

        font->draw("上下選戰場 | 左右換頁 | Tab切換", 12, 10, sh - 18, { 150, 150, 150, 200 });
        break;
    }
    case FreeBattleState::PreviewField:
    {
        int sw = Engine::getInstance()->getWindowWidth();
        int sh = Engine::getInstance()->getWindowHeight();
        auto font = Font::getInstance();
        
        // 半透明背景
        Engine::getInstance()->fillColor({ 0, 0, 0, 220 }, 0, 0, sw, sh);
        
        // 标题
        font->draw("地图预览", 24, sw / 2 - 60, 20, { 255, 215, 0, 255 });
        
        // 地图名称
        std::string field_name = "战场 " + std::to_string(preview_field_id_ + 1);
        font->draw(field_name, 20, sw / 2 - 50, 60, { 255, 255, 255, 255 });
        
        // 加载提示或显示地图
        if (preview_loading_)
        {
            font->draw("正在加载地图...", 16, sw / 2 - 70, sh / 2, { 200, 200, 200, 255 });
            preview_loading_ = false;
        }
        else if (preview_texture_)
        {
            // 显示地图纹理（缩放至合适大小）
            int map_w = 512, map_h = 512;
            int map_x = (sw - map_w) / 2;
            int map_y = 100;
            Engine::getInstance()->renderTexture(preview_texture_, map_x, map_y, map_w, map_h);
        }
        else
        {
            // 无预览图，显示占位符
            Engine::getInstance()->fillColor({ 50, 50, 80, 255 }, sw / 2 - 256, 100, 512, 512);
            font->draw("无预览图", 18, sw / 2 - 40, sh / 2, { 150, 150, 150, 255 });
        }
        
        // 提示信息
        font->draw("按 ESC 或 Enter 返回战场选择", 14, sw / 2 - 100, sh - 40, { 180, 180, 180, 200 });
        break;
    }
    case FreeBattleState::Ready:
    {
        int left_w = sw - getListWidth() - 30;
        drawSelectedTeam(10, content_y, left_w, friendly_ids_, friendly_count_, TEAMMATE_COUNT, "我方陣營", { 100, 200, 255, 255 });
        drawSelectedTeam(10, content_y + 220, left_w, enemy_ids_, enemy_count_, BATTLE_ENEMY_COUNT, "敵方陣營", { 255, 100, 100, 255 });

        // 战场信息
        std::string field_name = "戰場: 戰場 " + std::to_string(field_cursor_ + 1) + " / " + std::to_string(battle_field_count_);
        font->draw(field_name, 20, 10, content_y + 440, { 255, 215, 0, 255 });

        // 开战按钮（闪烁）
        int alpha = 180 + 75 * sin(select_flash_timer_ * 0.08);
        font->draw(">>> 開戰 <<<", 28, sw / 2 - 80, content_y + 420, { 255, 50, 50, (Uint8)alpha });

        font->draw("Enter: 開始戰鬥 | Tab: 返回", 12, 10, sh - 18, { 150, 150, 150, 200 });
        break;
    }
    default:
        break;
    }

    drawHint();
}

// ==================== 角色详情面板 ====================

void UIFreeBattle::drawRoleDetail(int x, int y, int w, int h, Role* role, Color team_color)
{
    // 存储面板位置给鼠标滚轮用
    detail_x_ = x;
    detail_y_ = y;
    detail_w_ = w;
    detail_h_ = h;

    auto font = Font::getInstance();
    Engine::getInstance()->fillColor({ 25, 25, 50, 220 }, x, y, w, h);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y, w, 1);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y + h - 1, w, 1);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y, 1, h);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x + w - 1, y, 1, h);

    if (!role)
    {
        font->draw("選擇角色查看詳情", 16, x + 10, y + h / 2 - 10, { 120, 120, 120, 200 });
        return;
    }

    // ===== Tab 栏 =====
    const char* tab_names[] = { "基本", "武功", "物品" };
    int tab_w = (w - 16) / 3;
    for (int i = 0; i < 3; i++)
    {
        Color tab_color = (i == detail_tab_) ? team_color : Color{ 60, 60, 60, 200 };
        Engine::getInstance()->fillColor(tab_color, x + 8 + i * (tab_w + 4), y + 6, tab_w, 22);
        font->draw(tab_names[i], 13, x + 8 + i * (tab_w + 4) + tab_w / 2 - 10, y + 10, { 255, 255, 255, 255 });
    }

    // ===== 内容区域 =====
    int content_y = y + 32;
    int content_h = h - 32;
    int px = x + 8;
    int row_h = 16;

    // 根据当前 Tab 计算内容总高度
    int total_h = 0;
    if (detail_tab_ == 0)
    {
        // 基本属性(8行) + 攻击属性(10行) + 其他(5行) + 头像+名字+分隔
        total_h = 80 + 20 + 6 + (8 + 10 + 5) * row_h + 3 * 24 + 20;
    }
    else if (detail_tab_ == 1)
    {
        int mc = 0, ic = 0;
        for (int i = 0; i < ROLE_MAGIC_COUNT; i++) if (role->MagicID[i] > 0) mc++;
        for (int i = 0; i < ROLE_INTERNAL_COUNT; i++) if (role->InternalID[i] > 0) ic++;
        total_h = 24 + mc * row_h + 8 + 24 + ic * row_h + 20;
    }
    else
    {
        int ic = 0;
        for (int i = 0; i < ROLE_TAKING_ITEM_COUNT; i++) if (role->TakingItem[i] > 0) ic++;
        total_h = 24 + ic * row_h + 20;
    }

    detail_scroll_max_ = std::max(0, total_h - content_h);
    detail_scroll_ = std::max(0, std::min(detail_scroll_, detail_scroll_max_));

    int py = content_y - detail_scroll_;

    auto row = [&](const std::string& label, const std::string& value, Color vc = { 220, 220, 220, 255 }) {
        font->draw(label, 12, px, py, { 160, 160, 160, 255 });
        font->draw(value, 12, px + 70, py, vc);
        py += row_h;
    };

    if (detail_tab_ == 0)
    {
        // 头像（缩小）
        TextureRenderInfo info;
        info.c = { 255, 255, 255, 255 };
        info.alpha = 255;
        info.zoom_x = 1.5;
        info.zoom_y = 1.5;
        TextureManager::getInstance()->renderTexture("head", role->HeadID, x + 20, py, info);
        py += 80;

        // 名字
        std::string name_line = std::string(role->Name) + "  Lv" + std::to_string(role->Level);
        font->draw(name_line, 16, x + 20, py, { 255, 255, 100, 255 });
        py += 20;

        py += 6;
        Engine::getInstance()->fillColor(team_color, px, py - 2, w - 16, 1);
        py += 6;

        // 基本属性
        font->draw("【 基本屬性 】", 13, px, py, { 255, 215, 0, 255 });
        py += 20;
        py += 4;
        row("生命", std::to_string(role->HP) + "/" + std::to_string(role->MaxHP), { 255, 100, 100, 255 });
        row("內力", std::to_string(role->MP) + "/" + std::to_string(role->MaxMP), { 100, 100, 255, 255 });
        row("攻擊", std::to_string(role->Attack));
        row("防禦", std::to_string(role->Defence));
        row("輕功", std::to_string(role->Speed));
        row("醫療", std::to_string(role->Medicine));
        row("用毒", std::to_string(role->UsePoison));
        row("解毒", std::to_string(role->Detoxification));
        row("抗毒", std::to_string(role->AntiPoison));

        py += 6;
        Engine::getInstance()->fillColor(team_color, px, py - 2, w - 16, 1);
        py += 6;

        // 攻击属性
        font->draw("【 攻擊屬性 】", 13, px, py, { 255, 215, 0, 255 });
        py += 20;
        py += 4;
        row("拳掌", std::to_string(role->Fist));
        row("御劍", std::to_string(role->Sword));
        row("耍刀", std::to_string(role->Knife));
        row("奇門", std::to_string(role->Unusual));
        row("暗器", std::to_string(role->HiddenWeapon));
        row("命中", std::to_string(role->HitRate) + "%");
        row("閃避", std::to_string(role->Dodge) + "%");
        row("暴擊", std::to_string(role->CriticalRate) + "%");
        row("聲望", std::to_string(role->Fame));
        row("道德", std::to_string(role->Morality));

        py += 6;
        Engine::getInstance()->fillColor(team_color, px, py - 2, w - 16, 1);
        py += 6;

        // 其他属性
        font->draw("【 其他屬性 】", 13, px, py, { 255, 215, 0, 255 });
        py += 20;
        py += 4;
        row("資質", std::to_string(role->IQ));
        row("學識", std::to_string(role->Knowledge));
        row("經驗", std::to_string(role->Exp));
        row("武力", std::to_string(role->AttackTwice) + "%");
        row("帶毒", std::to_string(role->AttackWithPoison) + "%");
    }
    else if (detail_tab_ == 1)
    {
        // 修炼武功
        font->draw("【 修煉武功 】", 13, px, py, { 255, 215, 0, 255 });
        py += 20;
        py += 4;
        bool has_magic = false;
        for (int i = 0; i < ROLE_MAGIC_COUNT; i++)
        {
            if (role->MagicID[i] > 0)
            {
                has_magic = true;
                auto* magic = Save::getInstance()->getMagic(role->MagicID[i]);
                std::string s = (magic ? std::string(magic->Name) : "(未命名)") + " Lv" + std::to_string(role->MagicLevel[i]);
                font->draw(s, 12, px, py, { 200, 220, 255, 255 });
                py += row_h;
            }
        }
        if (!has_magic)
        {
            font->draw("無", 12, px, py, { 120, 120, 120, 200 });
            py += row_h;
        }

        py += 6;
        Engine::getInstance()->fillColor(team_color, px, py - 2, w - 16, 1);
        py += 6;

        // 内功
        font->draw("【 內功 】", 13, px, py, { 255, 215, 0, 255 });
        py += 20;
        py += 4;
        bool has_internal = false;
        for (int i = 0; i < ROLE_INTERNAL_COUNT; i++)
        {
            if (role->InternalID[i] > 0)
            {
                has_internal = true;
                char buf[60];
                snprintf(buf, sizeof(buf), "內功ID:%d Lv%d", role->InternalID[i], role->InternalLevel[i]);
                font->draw(buf, 12, px, py, { 100, 255, 150, 255 });
                py += row_h;
            }
        }
        if (!has_internal)
        {
            font->draw("無", 12, px, py, { 120, 120, 120, 200 });
            py += row_h;
        }
    }
    else
    {
        // 携带物品
        font->draw("【 攜帶物品 】", 13, px, py, { 255, 215, 0, 255 });
        py += 20;
        py += 4;
        bool has_item = false;
        for (int i = 0; i < ROLE_TAKING_ITEM_COUNT; i++)
        {
            if (role->TakingItem[i] > 0)
            {
                has_item = true;
                auto* item = Save::getInstance()->getItem(role->TakingItem[i]);
                char buf[60];
                snprintf(buf, sizeof(buf), "%s x%d", item ? item->Name.c_str() : "(未命名)", role->TakingItemCount[i]);
                font->draw(buf, 12, px, py, { 255, 220, 150, 255 });
                py += row_h;
            }
        }
        if (!has_item)
        {
            font->draw("無", 12, px, py, { 120, 120, 120, 200 });
            py += row_h;
        }
    }

    // ===== 滚动条 =====
    if (detail_scroll_max_ > 0)
    {
        int sb_x = x + w - 12;
        int sb_y = content_y;
        int sb_h = content_h;
        int thumb_h = std::max(20, sb_h * sb_h / (sb_h + detail_scroll_max_));
        int thumb_y = sb_y + (sb_h - thumb_h) * detail_scroll_ / detail_scroll_max_;
        Engine::getInstance()->fillColor({ 50, 50, 50, 120 }, sb_x, sb_y, 6, sb_h);
        Engine::getInstance()->fillColor({ 180, 180, 180, 200 }, sb_x, thumb_y, 6, thumb_h);
    }
}
// ==================== 辅助方法 ====================

std::vector<Role*> UIFreeBattle::getFilteredRoles()
{
    if (search_text_.empty())
        return all_roles_;
    std::vector<Role*> result;
    for (auto* r : all_roles_)
    {
        std::string name = r->Name;
        std::string lower_search = search_text_;
        std::string lower_name = name;
        std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(lower_search) != std::string::npos)
            result.push_back(r);
    }
    return result;
}

void UIFreeBattle::addFriendly(int role_index)
{
    auto filtered = getFilteredRoles();
    if (role_index < 0 || role_index >= (int)filtered.size()) return;
    Role* r = filtered[role_index];
    if (!r) return;
    int rid = r->ID;
    if (friendly_count_ >= TEAMMATE_COUNT) { showHint("我方已滿"); return; }
    for (int i = 0; i < TEAMMATE_COUNT; i++)
        if (friendly_ids_[i] == rid) { showHint("該角色已在隊伍中"); return; }
    friendly_ids_[friendly_count_++] = rid;
    playSelectSound();
}

void UIFreeBattle::addEnemy(int role_index)
{
    auto filtered = getFilteredRoles();
    if (role_index < 0 || role_index >= (int)filtered.size()) return;
    Role* r = filtered[role_index];
    if (!r) return;
    int rid = r->ID;
    if (enemy_count_ >= BATTLE_ENEMY_COUNT) { showHint("敵方已滿"); return; }
    for (int i = 0; i < BATTLE_ENEMY_COUNT; i++)
        if (enemy_ids_[i] == rid) { showHint("該角色已在隊伍中"); return; }
    enemy_ids_[enemy_count_++] = rid;
    playSelectSound();
}

void UIFreeBattle::removeFriendly(int slot)
{
    if (slot < 0 || slot >= friendly_count_) return;
    for (int i = slot; i < TEAMMATE_COUNT - 1; i++)
        friendly_ids_[i] = friendly_ids_[i + 1];
    friendly_ids_[--friendly_count_] = -1;
    playSelectSound();
}

void UIFreeBattle::removeEnemy(int slot)
{
    if (slot < 0 || slot >= enemy_count_) return;
    for (int i = slot; i < BATTLE_ENEMY_COUNT - 1; i++)
        enemy_ids_[i] = enemy_ids_[i + 1];
    enemy_ids_[--enemy_count_] = -1;
    playSelectSound();
}

void UIFreeBattle::showHint(const std::string& text)
{
    hint_text_ = text;
    hint_timer_ = HINT_DURATION;
}

void UIFreeBattle::startBattle()
{
    if (friendly_count_ == 0) { showHint("請選擇我方角色"); return; }
    if (enemy_count_ == 0) { showHint("請選擇敵方角色"); return; }

    // 使用选中的战场索引作为 BattleFieldID（已在 battle_field_count_ 范围内）
    int battle_field_id = field_cursor_;

    // 填充 BattleInfo
    BattleInfo info;
    memset(&info, 0, sizeof(BattleInfo));
    // 初始化为空位（-1），使用 50v50 实际数组大小
    for (int i = 0; i < TEAMMATE_COUNT; i++) { info.TeamMate[i] = -1; info.TeamMateX[i] = -1; info.TeamMateY[i] = -1; }
    for (int i = 0; i < BATTLE_ENEMY_COUNT; i++) { info.Enemy[i] = -1; info.EnemyX[i] = -1; info.EnemyY[i] = -1; }
    info.BattleFieldID = battle_field_id;

    // 布局角色初始位置到地图中央区域（64×64 地图）
    const int MAP_C = BATTLEMAP_COORD_COUNT;  // 64
    const int CENTER_X = MAP_C / 2, CENTER_Y = MAP_C / 2;  // 32,32
    // 我方：中央偏左区域，网格排布
    if (friendly_count_ > 0)
    {
        int fc = std::min(friendly_count_, 5);   // 最多5列
        int fr = (friendly_count_ + fc - 1) / fc; // 行数
        int start_x = CENTER_X - (fc * 3) / 2 - 2;  // 中心偏左
        int start_y = CENTER_Y - (fr * 3) / 2;     // 垂直居中
        for (int i = 0; i < friendly_count_ && i < TEAMMATE_COUNT; i++)
        {
            int row = i / fc, col = i % fc;
            info.TeamMateX[i] = start_x + col * 3;
            info.TeamMateY[i] = start_y + row * 3;
        }
    }
    // 敌方：中央偏右区域，网格排布
    if (enemy_count_ > 0)
    {
        int ec = std::min(enemy_count_, 6);   // 最多6列
        int er = (enemy_count_ + ec - 1) / ec; // 行数
        int start_x = CENTER_X + 6;             // 中心偏右，留间隔
        int start_y = CENTER_Y - (er * 3) / 2;  // 垂直居中
        for (int i = 0; i < enemy_count_ && i < BATTLE_ENEMY_COUNT; i++)
        {
            int row = i / ec, col = i % ec;
            info.EnemyX[i] = start_x + col * 3;
            info.EnemyY[i] = start_y + row * 3;
        }
    }

    // 填充我方角色 ID（使用原版坐标，只替换 ID）
    for (int i = 0; i < friendly_count_; i++)
    {
        info.TeamMate[i] = friendly_ids_[i];
    }
    // 填充敌方角色 ID（使用原版坐标，只替换 ID）
    for (int i = 0; i < enemy_count_; i++)
    {
        info.Enemy[i] = enemy_ids_[i];
    }

    // 启动战斗（阻塞直到战斗结束，run() 返回 0=胜利 非0=失败）
    // 战斗模式跟随系统配置，未配置则默认黑帝斯
    std::shared_ptr<BattleScene> battle;
    std::string mode = GameUtil::getInstance()->getString("game", "battle_mode", "hades");
    if (mode == "sekiro")
        battle = std::make_shared<BattleSceneSekiro>();
    else if (mode == "paper")
        battle = std::make_shared<BattleScenePaper>();
    else
        battle = std::make_shared<BattleSceneHades>();  // 默认黑帝斯
    battle->setCustomBattleInfo(info);
    int result = battle->run();

    // 收集战斗统计
    collectBattleStats(battle.get());

    // 记录结果
    battle_winner_ = (result == 0) ? 0 : 1;
    state_ = FreeBattleState::BattleResult;
    result_page_ = 0;
}

// ==================== 角色列表 ====================

void UIFreeBattle::drawRoleList(int x, int y, int w, int h, const std::vector<Role*>& roles)
{
    auto font = Font::getInstance();
    Engine::getInstance()->fillColor({ 20, 20, 35, 220 }, x, y, w, h);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y, w, 1);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y + h - 1, w, 1);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y, 1, h);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x + w - 1, y, 1, h);

    int start = role_page_ * getRolePageSize();
    int end = std::min(start + getRolePageSize(), (int)roles.size());
    int row_h = 22;
    int line = 0;

    for (int i = start; i < end; i++, line++)
    {
        Role* r = roles[i];
        int ry = y + 4 + line * row_h;
        bool is_selected = (i == role_cursor_);
        bool flash = is_selected && (select_flash_timer_ % 30 < 15);

        if (is_selected)
        {
            if (flash)
                Engine::getInstance()->fillColor({ 80, 80, 120, 180 }, x + 2, ry, w - 4, row_h - 2);
            else
                Engine::getInstance()->fillColor({ 40, 40, 80, 180 }, x + 2, ry, w - 4, row_h - 2);
        }

        // 检查是否已在当前队伍中
        bool in_team = false;
        if (state_ == FreeBattleState::SelectFriendly)
        {
            for (int i = 0; i < friendly_count_; i++)
                if (friendly_ids_[i] == r->ID) { in_team = true; break; }
        }
        else
        {
            for (int i = 0; i < enemy_count_; i++)
                if (enemy_ids_[i] == r->ID) { in_team = true; break; }
        }

        if (r->HP <= 0)
            font->draw("(重傷)", 13, x + 6, ry + 4, { 180, 60, 60, 255 });
        else if (r->MP <= 0)
            font->draw("(無內)", 13, x + 6, ry + 4, { 60, 60, 180, 255 });
        else if (in_team)
            font->draw("●", 13, x + 6, ry + 4, { 100, 255, 100, 255 });  // 绿圈=已选入队伍
        else
            font->draw("○", 13, x + 6, ry + 4, { 200, 200, 200, 200 });

        std::string name = r->Name;
        if ((int)name.size() > 10) name = name.substr(0, 10) + "..";
        Color name_color = is_selected ? (flash ? Color{ 255, 255, 100, 255 } : Color{ 255, 220, 100, 255 }) : Color{ 210, 210, 210, 255 };
        font->draw(name, 13, x + 26, ry + 4, name_color);

        std::string hp_line = std::to_string(r->HP) + "/" + std::to_string(r->MaxHP);
        font->draw(hp_line, 12, x + 140, ry + 4, { 255, 100, 100, 255 });

        if (i < (int)roles.size())
            font->draw("Lv" + std::to_string(r->Level), 11, x + w - 38, ry + 4, { 160, 160, 160, 200 });
    }

    // 搜索框
    int sy = y + h - 28;
    Engine::getInstance()->fillColor({ 30, 30, 45, 220 }, x + 2, sy, w - 4, 24);
    if (search_active_)
    {
        std::string disp = ">" + search_text_ + "_";
        if ((select_flash_timer_ / 15) % 2 == 0)
            font->draw(disp, 14, x + 8, sy + 4, { 255, 255, 100, 255 });
        else
            font->draw(disp, 14, x + 8, sy + 4, { 200, 200, 80, 255 });
    }
    else
    {
        font->draw("[/] 搜索", 12, x + 8, sy + 4, { 130, 130, 130, 200 });
    }
}

// ==================== 阵容面板 ====================

// ==================== 阵容面板（完整版）====================

void UIFreeBattle::drawSelectedTeam(int x, int y, int w, int* ids, int count, int max_count, const std::string& title, Color color)
{
    auto font = Font::getInstance();
    Engine::getInstance()->fillColor({ 20, 20, 35, 220 }, x, y, w, 200);
    Engine::getInstance()->fillColor(color, x, y, w, 1);
    Engine::getInstance()->fillColor(color, x, y + 199, w, 1);
    Engine::getInstance()->fillColor(color, x, y, 1, 200);
    Engine::getInstance()->fillColor(color, x + w - 1, y, 1, 200);

    font->draw(title, 16, x + 8, y + 8, color);
    std::string cnt = std::to_string(count) + "/" + std::to_string(max_count);
    font->draw(cnt, 14, x + w - 50, y + 8, { 180, 180, 180, 255 });

    for (int i = 0; i < max_count; i++)
    {
        int ry = y + 32 + i * 20;
        if (i < count)
        {
            Role* r = Save::getInstance()->getRole(ids[i]);
            if (r)
            {
                font->draw(std::string(r->Name).substr(0, 12), 13, x + 8, ry, { 220, 220, 220, 255 });
                font->draw(std::to_string(r->HP) + "/" + std::to_string(r->MaxHP), 11, x + w - 80, ry, { 255, 100, 100, 255 });
            }
        }
        else
        {
            font->draw("---", 13, x + 8, ry, { 80, 80, 80, 150 });
        }
    }
}

// ==================== 阵容面板（紧凑版，用于选择界面左侧）====================

void UIFreeBattle::drawSelectedTeamCompact(int x, int y, int w, int h, int* ids, int count, int max_count, const std::string& title, Color color)
{
    auto font = Font::getInstance();
    Engine::getInstance()->fillColor({ 20, 20, 35, 220 }, x, y, w, h);
    Engine::getInstance()->fillColor(color, x, y, w, 1);
    Engine::getInstance()->fillColor(color, x, y + h - 1, w, 1);
    Engine::getInstance()->fillColor(color, x, y, 1, h);
    Engine::getInstance()->fillColor(color, x + w - 1, y, 1, h);

    // 标题栏
    font->draw(title, 15, x + 8, y + 8, color);
    std::string cnt = std::to_string(count) + "/" + std::to_string(max_count);
    font->draw(cnt, 13, x + w - 60, y + 8, { 180, 180, 180, 255 });
    Engine::getInstance()->fillColor(color, x, y + 28, w, 1);

    if (count == 0)
    {
        font->draw("(未選擇)", 13, x + 8, y + 38, { 100, 100, 100, 180 });
        return;
    }

    int row_h = 18;
    int max_rows = (h - 36) / row_h;
    for (int i = 0; i < count && i < max_rows; i++)
    {
        int ry = y + 36 + i * row_h;
        Role* r = Save::getInstance()->getRole(ids[i]);
        if (r)
        {
            std::string name = std::string(r->Name).substr(0, 10);
            bool highlight = false;
            if (state_ == FreeBattleState::SelectFriendly)
            {
                auto filtered = getFilteredRoles();
                if (role_cursor_ >= 0 && role_cursor_ < (int)filtered.size() && filtered[role_cursor_]->ID == r->ID)
                    highlight = true;
            }
            Color name_color = highlight ? Color{ 255, 255, 100, 255 } : Color{ 200, 200, 200, 255 };
            font->draw(name, 13, x + 8, ry, name_color);
            font->draw(std::to_string(r->HP) + "/" + std::to_string(r->MaxHP), 11, x + w - 80, ry, { 255, 100, 100, 255 });
        }
    }
}

// ==================== 战场列表 ====================

void UIFreeBattle::drawBattleFieldList(int x, int y, int w, int h)
{
    auto font = Font::getInstance();
    Engine::getInstance()->fillColor({ 20, 20, 35, 220 }, x, y, w, h);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y, w, 1);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y + h - 1, w, 1);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x, y, 1, h);
    Engine::getInstance()->fillColor({ 80, 80, 120, 200 }, x + w - 1, y, 1, h);

    int start = field_page_ * getFieldPageSize();
    int end = std::min(start + getFieldPageSize(), battle_field_count_);
    int row_h = 28;
    int line = 0;

    for (int i = start; i < end; i++, line++)
    {
        int ry = y + 4 + line * row_h;
        bool is_selected = (i == field_cursor_);
        bool flash = is_selected && (select_flash_timer_ % 30 < 15);

        if (is_selected)
        {
            if (flash)
                Engine::getInstance()->fillColor({ 80, 80, 120, 180 }, x + 2, ry, w - 4, row_h - 2);
            else
                Engine::getInstance()->fillColor({ 40, 40, 80, 180 }, x + 2, ry, w - 4, row_h - 2);
        }

        std::string name = "戰場 " + std::to_string(i + 1);
        Color name_color = is_selected ? (flash ? Color{ 255, 255, 100, 255 } : Color{ 255, 220, 100, 255 }) : Color{ 210, 210, 210, 255 };
        font->draw(name, 14, x + 8, ry + 6, name_color);
    }
}

// ==================== 分页 ====================

void UIFreeBattle::drawPagination(int x, int y, int page, int total_pages)
{
    if (total_pages <= 1) return;
    auto font = Font::getInstance();
    std::string text = "第 " + std::to_string(page + 1) + " / " + std::to_string(total_pages) + " 頁";
    font->draw(text, 12, x, y, { 160, 160, 160, 200 });
}

// ==================== 提示 ====================

void UIFreeBattle::drawHint()
{
    if (hint_timer_ <= 0) return;
    hint_timer_--;
    auto font = Font::getInstance();
    int alpha = (hint_timer_ > 30) ? 255 : (hint_timer_ * 255 / 30);
    int sw = Engine::getInstance()->getWindowWidth();
    int w = std::min((int)hint_text_.size() * 16 + 40, sw - 40);
    int x = (sw - w) / 2;
    int y = Engine::getInstance()->getWindowHeight() - 80;
    Engine::getInstance()->fillColor({ 0, 0, 0, (Uint8)alpha }, x, y, w, 30);
    font->draw(hint_text_, 16, x + 10, y + 6, { 255, 200, 80, (Uint8)alpha });
}

// ==================== 战斗结算 ====================

void UIFreeBattle::drawBattleResult()
{
    int sw = Engine::getInstance()->getWindowWidth();
    int sh = Engine::getInstance()->getWindowHeight();
    auto font = Font::getInstance();

    Engine::getInstance()->fillColor({ 0, 0, 0, 230 }, 0, 0, sw, sh);

    std::string title = (battle_winner_ == 0) ? "【 我方勝利 】" : "【 敵方勝利 】";
    Color title_color = (battle_winner_ == 0) ? Color{ 100, 255, 100, 255 } : Color{ 255, 80, 80, 255 };
    font->draw(title, 28, sw / 2 - 80, 20, title_color);

    // Tab
    const char* pages[] = { "總覽", "我方", "敵方" };
    int tab_w = 80;
    for (int i = 0; i < 3; i++)
    {
        int tx = sw / 2 - 120 + i * (tab_w + 4);
        Color tc = (result_page_ == i) ? Color{ 255, 215, 0, 255 } : Color{ 150, 150, 150, 200 };
        Engine::getInstance()->fillColor(tc, tx, 58, tab_w, 24);
        font->draw(pages[i], 14, tx + 18, 62, { 0, 0, 0, 255 });
    }

    // 计算本页显示的角色
    std::vector<BattleRoleStats*> display;
    for (auto& s : battle_stats_)
    {
        if (result_page_ == 0) display.push_back(&s);
        else if (result_page_ == 1 && s.team == 0) display.push_back(&s);
        else if (result_page_ == 2 && s.team == 1) display.push_back(&s);
    }

    int col = 0;
    int col_w = sw / 2 - 20;
    int row_y = 90;
    int row_h = 24;
    for (size_t i = 0; i < display.size(); i++)
    {
        auto* s = display[i];
        int rx = (col == 0) ? 10 : sw / 2 + 10;
        int ry = row_y + (int)i * row_h;
        if (ry + row_h > sh - 50) { col = 1; rx = sw / 2 + 10; ry = row_y; }

        std::string line = s->name + " ";
        line += std::to_string(s->damage_dealt) + "傷 " + std::to_string(s->action_count) + "動";
        if (s->kill_count > 0) line += " " + std::to_string(s->kill_count) + "殺";
        line += " " + std::to_string(s->final_hp) + "/" + std::to_string(s->max_hp) + "HP";

        Color rc = s->survived ? Color{ 200, 200, 200, 255 } : Color{ 160, 60, 60, 255 };
        font->draw(line, 13, rx, ry, rc);
    }

    int alpha = 180 + 75 * sin(select_flash_timer_ * 0.08);
    font->draw("按 Enter 返回主選單", 16, sw / 2 - 90, sh - 40, { 200, 200, 200, (Uint8)alpha });
}

void UIFreeBattle::collectBattleStats(BattleScene* scene)
{
    battle_stats_.clear();
    auto& roles = scene->getBattleRoles();
    for (Role* r : roles)
    {
        if (!r) continue;
        BattleRoleStats s;
        s.role_id = r->ID;
        s.name = r->Name;
        // TODO: 判断 team（需要区分我方/敌方）
        s.team = r->Team;  // 0=我方，1=敌方
        s.damage_dealt = r->damage_dealt;       // 造成伤害（方案A）
        s.damage_taken = r->damage_taken;       // 受到伤害（方案A）
        s.heal_done = r->healing_done;          // 治疗量（方案A）
        s.poison_damage = r->poison_damage;     // 毒伤（方案A）
        s.kill_count = r->kill_count;            // 击杀数（方案A）
        s.action_count = r->action_count;        // 行动次数（方案A）
        s.survived = r->HP > 0;
        s.max_hp = r->MaxHP;
        s.final_hp = r->HP;
        battle_stats_.push_back(s);
    }
}

// ==================== 事件处理 ====================

void UIFreeBattle::dealEvent(EngineEvent& e)
{
    // ===== 鼠标点击 =====
    if (e.type == EVENT_MOUSE_BUTTON_DOWN && e.button.button == 1)
    {
        int mx = e.button.x;
        int my = e.button.y;
        bool handled = false;

        // ① 顶部状态 Tab（我方/敵方/戰場/準備）
        {
            int sw = Engine::getInstance()->getWindowWidth();
            int tab_w = 90;
            int tab_start_x = sw / 2 - 2 * tab_w;
            if (my >= 36 && my <= 54 && mx >= tab_start_x && mx < tab_start_x + 4 * tab_w)
            {
                for (int i = 0; i < 4; i++)
                {
                    int tx = tab_start_x + i * tab_w;
                    if (mx >= tx && mx < tx + tab_w)
                    {
                        FreeBattleState new_st = (FreeBattleState)i;
                        if (state_ != new_st)
                        {
                            if (new_st == FreeBattleState::Ready &&
                                (friendly_count_ == 0 || enemy_count_ == 0))
                            {
                                showHint("請先選擇雙方角色");
                            }
                            else
                            {
                                state_ = new_st;
                                if (state_ == FreeBattleState::SelectField)
                                {
                                    field_cursor_ = 0;
                                    field_page_ = 0;
                                }
                                playSelectSound();
                            }
                        }
                        handled = true;
                        break;
                    }
                }
            }
        }

        // ② 详情面板 Tab（基本/武功/物品）
        if (!handled && detail_w_ > 0 &&
            (state_ == FreeBattleState::SelectFriendly || state_ == FreeBattleState::SelectEnemy))
        {
            if (my >= detail_y_ + 6 && my <= detail_y_ + 28 &&
                mx >= detail_x_ + 8 && mx <= detail_x_ + 8 + detail_w_)
            {
                int tw = (detail_w_ - 16) / 3;
                for (int i = 0; i < 3; i++)
                {
                    int tab_x = detail_x_ + 8 + i * (tw + 4);
                    if (mx >= tab_x && mx < tab_x + tw)
                    {
                        if (detail_tab_ != i)
                        {
                            detail_tab_ = i;
                            detail_scroll_ = 0;
                            detail_scroll_max_ = 0;
                            playSelectSound();
                        }
                        handled = true;
                        break;
                    }
                }
            }
        }

        // ③ 角色列表点击选中 + 双击加入
        if (!handled && (state_ == FreeBattleState::SelectFriendly || state_ == FreeBattleState::SelectEnemy))
        {
            int sw = Engine::getInstance()->getWindowWidth();
            int lw = getListWidth();
            int ry = 60;
            int rh = Engine::getInstance()->getWindowHeight() - ry - 30;
            int rx = sw - lw - 10;
            if (mx >= rx && mx < rx + lw && my >= ry && my < ry + rh)
            {
                auto filtered = getFilteredRoles();
                int row_h = 22;
                int start_line = role_page_ * getRolePageSize();
                int rel_y = my - (ry + 4);
                int line = rel_y / row_h;
                int idx = start_line + line;
                if (idx >= 0 && idx < (int)filtered.size())
                {
                    role_cursor_ = idx;
                    Uint32 now = SDL_GetTicks();
                    if (last_click_idx_ == idx && (int)(now - last_click_ticks_) < 300)
                    {
                        if (state_ == FreeBattleState::SelectFriendly)
                            addFriendly(role_cursor_);
                        else
                            addEnemy(role_cursor_);
                        last_click_idx_ = -1;
                    }
                    else
                    {
                        last_click_idx_ = idx;
                        last_click_ticks_ = now;
                    }
                    playSelectSound();
                    handled = true;
                }
            }
        }

        // ④ 战场列表点击选中
        if (!handled && state_ == FreeBattleState::SelectField)
        {
            int sw = Engine::getInstance()->getWindowWidth();
            int lw = getListWidth();
            int ry = 60;
            int rh = Engine::getInstance()->getWindowHeight() - ry - 30;
            int rx = sw - lw - 10;
            if (mx >= rx && mx < rx + lw && my >= ry && my < ry + rh)
            {
                int row_h = 22;
                int start_line = field_page_ * getFieldPageSize();
                int rel_y = my - (ry + 4);
                int line = rel_y / row_h;
                int idx = start_line + line;
                if (idx >= 0 && idx < battle_field_count_)
                {
                    field_cursor_ = idx;
                    playSelectSound();
                    handled = true;
                }
            }
        }

        // ⑤ 翻页箭头点击
        if (!handled)
        {
            int sw = Engine::getInstance()->getWindowWidth();
            int lw = getListWidth();
            int py = Engine::getInstance()->getWindowHeight() - 28;
            int px = sw - lw - 10;
            int pw = lw;
            // 左箭头 ⟨
            if (mx >= px && mx < px + 30 && my >= py && my < py + 20)
            {
                if (state_ == FreeBattleState::SelectFriendly || state_ == FreeBattleState::SelectEnemy)
                {
                    if (role_page_ > 0) { role_page_--; role_cursor_ = role_page_ * getRolePageSize(); }
                }
                else if (state_ == FreeBattleState::SelectField)
                {
                    if (field_page_ > 0) { field_page_--; field_cursor_ = field_page_ * getFieldPageSize(); }
                }
                playSelectSound();
                handled = true;
            }
            // 右箭头 ⟩
            if (!handled && mx >= px + pw - 30 && mx < px + pw && my >= py && my < py + 20)
            {
                if (state_ == FreeBattleState::SelectFriendly || state_ == FreeBattleState::SelectEnemy)
                {
                    if (role_page_ + 1 < getRolePageCount()) { role_page_++; role_cursor_ = role_page_ * getRolePageSize(); }
                }
                else if (state_ == FreeBattleState::SelectField)
                {
                    if (field_page_ + 1 < getFieldPageCount()) { field_page_++; field_cursor_ = field_page_ * getFieldPageSize(); }
                }
                playSelectSound();
                handled = true;
            }
        }

        // ⑥ 准备界面「開戰」按钮
        if (!handled && state_ == FreeBattleState::Ready)
        {
            int sw = Engine::getInstance()->getWindowWidth();
            int bh_x = sw / 2 - 80;
            int bh_y = 60 + 420;
            int bh_w = 160;
            int bh_h = 40;
            if (mx >= bh_x && mx < bh_x + bh_w && my >= bh_y && my < bh_y + bh_h)
            {
                startBattle();
                playSelectSound();
                handled = true;
            }
        }

        if (handled) return;
    }

    // ===== 鼠标滚轮 =====
    if (e.type == EVENT_MOUSE_WHEEL)
    {
        float fmx, fmy;
        SDL_GetMouseState(&fmx, &fmy);
        int mx = (int)fmx;
        int my = (int)fmy;
        if (mx >= detail_x_ && mx < detail_x_ + detail_w_ && my >= detail_y_ && my < detail_y_ + detail_h_)
        {
            int delta = e.wheel.y * 20;
            detail_scroll_ = std::max(0, std::min(detail_scroll_ + delta, detail_scroll_max_));
            return;
        }
    }

    // ===== 搜索模式 =====
    if (search_active_)
    {
        if (e.type == EVENT_KEY_UP && e.key.key == K_ESCAPE)
        {
            search_active_ = false;
            search_text_.clear();
            return;
        }
        if (e.type == EVENT_KEY_UP && e.key.key == K_RETURN)
        {
            search_active_ = false;
            return;
        }
        if (e.type == EVENT_TEXT_INPUT)
        {
            search_text_ += e.text.text;
            if (search_text_.size() > 12) search_text_.resize(12);
            return;
        }
        return;
    }

    if (e.type == EVENT_KEY_UP && e.key.key == SDLK_SLASH)
    {
        search_active_ = true;
        search_text_.clear();
        return;
    }

    // ===== 战斗结果 =====
    if (state_ == FreeBattleState::BattleResult)
    {
        if (e.type == EVENT_KEY_UP && (e.key.key == K_RETURN || e.key.key == K_ESCAPE))
        {
            // 返回角色选择界面
            state_ = FreeBattleState::SelectFriendly;
            friendly_count_ = 0;
            enemy_count_ = 0;
            battle_winner_ = -1;
            result_page_ = 0;
            battle_stats_.clear();
            return;
        }
        if (e.type == EVENT_KEY_UP && e.key.key == K_LEFT)
        {
            result_page_ = (result_page_ + 2) % 3;
            playSelectSound();
        }
        else if (e.type == EVENT_KEY_UP && e.key.key == K_RIGHT)
        {
            result_page_ = (result_page_ + 1) % 3;
            playSelectSound();
        }
        return;
    }

    // ===== TAB 切换状态 =====
    if (e.type == EVENT_KEY_UP && e.key.key == K_TAB)
    {
        if (state_ == FreeBattleState::SelectFriendly)
        {
            state_ = FreeBattleState::SelectEnemy;
            playSelectSound();
        }
        else if (state_ == FreeBattleState::SelectEnemy)
        {
            state_ = FreeBattleState::SelectField;
            field_cursor_ = 0;
            field_page_ = 0;
            playSelectSound();
        }
        else if (state_ == FreeBattleState::SelectField)
        {
            // 只有双方都选了人，才能进入准备界面
            if (friendly_count_ > 0 && enemy_count_ > 0)
            {
                state_ = FreeBattleState::Ready;
            }
            else
            {
                state_ = FreeBattleState::SelectFriendly;
            }
            playSelectSound();
        }
        else if (state_ == FreeBattleState::Ready)
        {
            // 返回初始状态，清空选择
            state_ = FreeBattleState::SelectFriendly;
            friendly_count_ = 0;
            enemy_count_ = 0;
            playSelectSound();
        }
        return;
    }

    // ===== 按状态分派 =====
    switch (state_)
    {
    case FreeBattleState::SelectFriendly:
    case FreeBattleState::SelectEnemy:
    {
        auto filtered = getFilteredRoles();
        if (e.type == EVENT_KEY_UP)
        {
            switch (e.key.key)
            {
            case K_UP:
                role_cursor_ = (role_cursor_ - 1 + (int)filtered.size()) % filtered.size();
                role_page_ = role_cursor_ / getRolePageSize();
                playSelectSound();
                break;
            case K_DOWN:
                role_cursor_ = (role_cursor_ + 1) % filtered.size();
                role_page_ = role_cursor_ / getRolePageSize();
                playSelectSound();
                break;
            case K_LEFT:
                if (role_page_ > 0) { role_page_--; role_cursor_ = role_page_ * getRolePageSize(); }
                break;
            case K_RIGHT:
                if (role_page_ + 1 < getRolePageCount()) { role_page_++; role_cursor_ = role_page_ * getRolePageSize(); }
                break;
            case K_RETURN:
            case K_SPACE:
            {
                auto filtered = getFilteredRoles();
                if (role_cursor_ >= 0 && role_cursor_ < (int)filtered.size())
                {
                    Role* r = filtered[role_cursor_];
                    if (r)
                    {
                        int rid = r->ID;
                        int* ids = (state_ == FreeBattleState::SelectFriendly) ? friendly_ids_ : enemy_ids_;
                        int& count = (state_ == FreeBattleState::SelectFriendly) ? friendly_count_ : enemy_count_;
                        int found_idx = -1;
                        for (int i = 0; i < count; i++)
                        {
                            if (ids[i] == rid) { found_idx = i; break; }
                        }
                        if (found_idx >= 0)
                        {
                            // 已在队伍中 → 移除
                            if (state_ == FreeBattleState::SelectFriendly)
                                removeFriendly(found_idx);
                            else
                                removeEnemy(found_idx);
                            showHint("已移除: " + std::string(r->Name));
                        }
                        else
                        {
                            // 不在队伍中 → 添加
                            if (state_ == FreeBattleState::SelectFriendly)
                                addFriendly(role_cursor_);
                            else
                                addEnemy(role_cursor_);
                        }
                    }
                }
                break;
            }
            case K_DELETE:
            case K_BACKSPACE:
                if (state_ == FreeBattleState::SelectFriendly && friendly_count_ > 0)
                    removeFriendly(friendly_count_ - 1);
                else if (state_ == FreeBattleState::SelectEnemy && enemy_count_ > 0)
                    removeEnemy(enemy_count_ - 1);
                break;
            case K_ESCAPE:
                if (friendly_count_ > 0 && enemy_count_ > 0)
                {
                    state_ = FreeBattleState::SelectField;
                    field_cursor_ = 0;
                    field_page_ = 0;
                    playSelectSound();
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    case FreeBattleState::SelectField:
    {
        if (e.type == EVENT_KEY_UP)
        {
            switch (e.key.key)
            {
            case K_UP:
                field_cursor_ = (field_cursor_ - 1 + battle_field_count_) % battle_field_count_;
                field_page_ = field_cursor_ / getFieldPageSize();
                playSelectSound();
                break;
            case K_DOWN:
                field_cursor_ = (field_cursor_ + 1) % battle_field_count_;
                field_page_ = field_cursor_ / getFieldPageSize();
                playSelectSound();
                break;
            case K_LEFT:
                if (field_page_ > 0) { field_page_--; field_cursor_ = field_page_ * getFieldPageSize(); }
                break;
            case K_RIGHT:
                if (field_page_ + 1 < getFieldPageCount()) { field_page_++; field_cursor_ = field_page_ * getFieldPageSize(); }
                break;
            case K_RETURN:
            case K_SPACE:
            {
                if (friendly_count_ > 0 && enemy_count_ > 0)
                {
                    state_ = FreeBattleState::Ready;
                    playSelectSound();
                }
                else showHint("請先選擇雙方角色");
                break;
            }
            case K_P:  // 预览地图
            {
                state_ = FreeBattleState::PreviewField;
                preview_field_id_ = field_cursor_;
                preview_loading_ = true;
                playSelectSound();
                break;
            }
            case K_ESCAPE:
                state_ = FreeBattleState::SelectEnemy;
                playSelectSound();
                break;
            default:
                break;
            }
        }
        break;
    }
    
    case FreeBattleState::PreviewField:
    {
        if (e.type == EVENT_KEY_UP)
        {
            switch (e.key.key)
            {
            case K_RETURN:
            case K_ESCAPE:
                state_ = FreeBattleState::SelectField;
                playSelectSound();
                break;
            default:
                break;
            }
        }
        break;
    }
    
    case FreeBattleState::Ready:
    {
        if (e.type == EVENT_KEY_UP && e.key.key == K_RETURN)
        {
            startBattle();
            playSelectSound();
        }
        else if (e.type == EVENT_KEY_UP && (e.key.key == K_ESCAPE || e.key.key == K_TAB))
        {
            state_ = FreeBattleState::SelectField;
            playSelectSound();
        }
        break;
    }
    default:
        break;
    }
}
void UIFreeBattle::backRun()
{
    // 场景切换后回填结算
}

void UIFreeBattle::onPressedOK()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_UP;
    e.key.key = SDLK_RETURN;
    e.key.scancode = SDL_SCANCODE_RETURN;
    dealEvent(e);
}

void UIFreeBattle::onPressedCancel()
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_EVENT_KEY_UP;
    e.key.key = SDLK_ESCAPE;
    e.key.scancode = SDL_SCANCODE_ESCAPE;
    dealEvent(e);
}
