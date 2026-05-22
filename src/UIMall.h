#pragma once
#include "RunNode.h"
#include "Types.h"
#include "Save.h"
#include "Font.h"
#include "Button.h"
#include "GameUtil.h"
#include "Audio.h"
#include <cstdint>

// 商城物品分类
enum class MallCategory : int {
    剧情道具 = 0,
    兵器,
    护甲,
    丹药,
    暗器,
    拳经,
    剑谱,
    刀录,
    奇门,
    心法,
    COUNT
};

class UIMall : public RunNode {
public:
    UIMall();
    ~UIMall() override = default;

    void setRole(Role* r) { current_role_ = r; }

    void draw() override;
    void dealEvent(EngineEvent& e) override;
    void backRun() override;
    void onEntrance() override;
    void onExit() override;

    void onPressedOK() override;
    void onPressedCancel() override;

    // 获取物品所属分类
    static int getItemCategory(Item* item);
    // 获取分类名称
    static std::string getCategoryName(int cat);
    // 获取某分类下的所有物品
    static std::vector<Item*> getItemsByCategory(int cat);
    // 计算物品价格（根据贵重/稀有权重）
    static int calculateItemPrice(Item* item);

private:
    Role* current_role_ = nullptr;
    int current_category_ = 0;   // 当前选中的分类
    int scroll_offset_ = 0;      // 物品列表滚动偏移
    int selected_index_ = 0;      // 当前选中的物品索引（在分类列表内）
    int buy_count_ = 1;           // 购买数量

    std::vector<Item*> category_items_;  // 当前分类的物品列表

    std::shared_ptr<Button> btn_buy_;
    std::shared_ptr<Button> btn_plus_;
    std::shared_ptr<Button> btn_minus_;
    std::shared_ptr<Button> btn_ok_;
    std::shared_ptr<Button> btn_cancel_;

    void refreshCategoryItems();
    void drawCategoryTabs(int x, int y);
    void drawItemList(int x, int y);
    void drawItemDetail(int x, int y);
    void drawBuyPanel(int x, int y);
    void showHint(const std::string& text, Color color, int sound_id = -1);
    void drawHint();

    // 提示信息（自动消失）
    std::string hint_text_;
    Color hint_color_{255, 255, 255, 255};
    uint32_t hint_timer_ = 0;       // 提示显示剩余帧数（60fps，2秒=120帧）
    static constexpr int HINT_DURATION = 120;  // 2秒
};
