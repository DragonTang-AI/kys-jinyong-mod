#pragma once
#include "RunNode.h"
#include "Types.h"
#include "Save.h"
#include "Font.h"
#include "Button.h"
#include "GameUtil.h"
#include "Audio.h"
#include "BattleMap.h"
#include "Event.h"
#include <vector>
#include <string>
#include <map>

// 自由对战界面状态
enum class FreeBattleState : int {
    SelectFriendly = 0,   // 选择我方角色
    SelectEnemy = 1,      // 选择敌方角色
    SelectField = 2,      // 选择战场
    Ready = 3,            // 确认开战
    BattleResult = 4      // 战斗结算
};

// 角色战斗统计
struct BattleRoleStats {
    int role_id = -1;
    std::string name;
    int team = 0;           // 0=我方, 1=敌方
    int damage_dealt = 0;   // 造成伤害
    int damage_taken = 0;   // 受到伤害
    int heal_done = 0;      // 治疗量
    int poison_damage = 0;  // 毒伤（方案A）
    int kill_count = 0;     // 击杀数
    int action_count = 0;   // 行动次数
    bool survived = false;  // 是否存活
    int max_hp = 0;         // 最大HP
    int final_hp = 0;       // 战斗结束时HP
};

class UIFreeBattle : public RunNode {
public:
    UIFreeBattle();
    ~UIFreeBattle() override = default;

    void draw() override;
    void dealEvent(EngineEvent& e) override;
    void backRun() override;
    void onEntrance() override;
    void onPressedOK() override;
    void onPressedCancel() override;

    int getResult() const { return result_; }

private:
    FreeBattleState state_ = FreeBattleState::SelectFriendly;

    // 角色列表
    std::vector<Role*> all_roles_;
    int role_cursor_ = 0;         // 当前选中角色索引
    int role_page_ = 0;           // 角色列表当前页码（0-based）

    // 我方已选角色ID
    int friendly_ids_[TEAMMATE_COUNT] = {};
    int friendly_count_ = 0;

    // 敌方已选角色ID
    int enemy_ids_[BATTLE_ENEMY_COUNT] = {};
    int enemy_count_ = 0;

    // 战场选择
    int field_cursor_ = 0;
    int field_page_ = 0;           // 战场列表当前页码
    int battle_field_count_ = 0;

    // 结果
    int result_ = -1;

    // 搜索
    std::string search_text_;
    bool search_active_ = false;

    // 提示
    std::string hint_text_;
    int hint_timer_ = 0;
    const int HINT_DURATION = 120;

    // 选中闪烁动画
    int select_flash_timer_ = 0;

    // 战斗结算
    int battle_winner_ = -1;       // 0=我方胜, 1=敌方胜
    std::vector<BattleRoleStats> battle_stats_;
    int result_page_ = 0;         // 结算页码（0=总览, 1=我方详细, 2=敌方详细）

    // 详情面板滚动
    int detail_scroll_ = 0;       // 滚动偏移（像素，向下为正）
    int detail_scroll_max_ = 0;   // 最大滚动范围
    int detail_tab_ = 0;          // 0=属性, 1=武功, 2=物品

    // 双击检测（鼠标点击角色列表）
    int last_click_idx_ = -1;      // 上次单击的角色索引
    Uint32 last_click_ticks_ = 0;  // 上次单击的 SDL ticks

    // 详情面板尺寸（draw() 中计算）
    int detail_x_ = 0, detail_y_ = 0, detail_w_ = 0, detail_h_ = 0;

    // 分页参数
    int getRolePageSize() const { return 14; }
    int getFieldPageSize() const { return 12; }
    int getRolePageCount() const;
    int getFieldPageCount() const;
    int getVisibleLineCount() const { return 14; }
    int getListWidth() const { return 300; }

    // 辅助方法
    void drawRoleList(int x, int y, int w, int h, const std::vector<Role*>& roles);
    void drawRoleDetail(int x, int y, int w, int h, Role* role, Color team_color);
    void drawSelectedTeam(int x, int y, int w, int* ids, int count, int max_count, const std::string& title, Color color);
    void drawSelectedTeamCompact(int x, int y, int w, int h, int* ids, int count, int max_count, const std::string& title, Color color);
    void drawBattleFieldList(int x, int y, int w, int h);
    void drawHint();
    void drawPagination(int x, int y, int page, int total_pages);
    void drawBattleResult();
    void showHint(const std::string& text);

    std::vector<Role*> getFilteredRoles();

    void addFriendly(int role_index);
    void addEnemy(int role_index);
    void removeFriendly(int slot);
    void removeEnemy(int slot);
    void startBattle();
    void collectBattleStats(class BattleScene* scene);
    void playSelectSound();
};
