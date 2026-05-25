#include "UIMall.h"
#include "Engine.h"
#include "TextureManager.h"
#include "Event.h"
#include "../mlcc/PotConv.h"
#include <format>
#include <chrono>
#include <algorithm>
#include <cmath>

UIMall::UIMall() {
    btn_buy_ = std::make_shared<Button>();
    btn_buy_->setText("購買");
    btn_buy_->setFontSize(20);
    addChild(btn_buy_, 400, 520);

    btn_plus_ = std::make_shared<Button>();
    btn_plus_->setText("＋");
    btn_plus_->setFontSize(20);
    addChild(btn_plus_, 500, 520);

    btn_minus_ = std::make_shared<Button>();
    btn_minus_->setText("﹣");
    btn_minus_->setFontSize(20);
    addChild(btn_minus_, 550, 520);

    btn_ok_ = std::make_shared<Button>();
    btn_ok_->setText("確認");
    btn_ok_->setFontSize(20);
    addChild(btn_ok_, 650, 520);

    btn_cancel_ = std::make_shared<Button>();
    btn_cancel_->setText("取消");
    btn_cancel_->setFontSize(20);
    addChild(btn_cancel_, 750, 520);

    setPosition(200, 40);
    refreshCategoryItems();
}

void UIMall::onEntrance() {
    refreshCategoryItems();
    selected_index_ = 0;
    scroll_offset_ = 0;
    buy_count_ = 1;
}

void UIMall::onExit() {
}

// 获取物品所属分类（枚举使用简体，与 UIMall.h 一致）
int UIMall::getItemCategory(Item* item) {
    if (!item) return -1;
    switch (item->ItemType) {
        case 0: return (int)MallCategory::剧情道具;
        case 1:
            if (item->EquipType == 0) return (int)MallCategory::兵器;
            else return (int)MallCategory::护甲;
        case 2: {
            std::string name = item->Name;
            // item->Name is CP936 encoded; convert search keywords to CP936 at runtime
            if (name.find("拳") != std::string::npos) return (int)MallCategory::拳经;
            if (name.find("剑") != std::string::npos) return (int)MallCategory::剑谱;
            if (name.find("刀") != std::string::npos) return (int)MallCategory::刀录;
            if (name.find("心法") != std::string::npos) return (int)MallCategory::心法;
            return (int)MallCategory::奇门;
        }
        case 3: return (int)MallCategory::丹药;
        case 4: return (int)MallCategory::暗器;
        default: return (int)MallCategory::剧情道具;
    }
}

// 分类显示名称（使用繁体，符合游戏UI风格）
std::string UIMall::getCategoryName(int cat) {
    switch ((MallCategory)cat) {
        case MallCategory::剧情道具: return "劇情道具";
        case MallCategory::兵器:   return "兵器";
        case MallCategory::护甲:   return "護甲";
        case MallCategory::丹药:   return "丹藥";
        case MallCategory::暗器:   return "暗器";
        case MallCategory::拳经:   return "拳經";
        case MallCategory::剑谱:   return "劍譜";
        case MallCategory::刀录:   return "刀錄";
        case MallCategory::奇门:   return "奇門";
        case MallCategory::心法:   return "心法";
        default: return "未知";
    }
}

// 获取某分类下的所有物品
std::vector<Item*> UIMall::getItemsByCategory(int cat) {
    std::vector<Item*> result;
    auto* save = Save::getInstance();
    for (int i = 0; i < 512; i++) {
        Item* item = save->getItem(i);
        if (!item) continue;
        if (getItemCategory(item) == cat) {
            result.push_back(item);
        }
    }
    return result;
}

// 根据贵重/稀有权重计算物品价格
int UIMall::calculateItemPrice(Item* item) {
    if (!item) return 0;
    int cat = getItemCategory(item);

    int price = 0;
    switch ((MallCategory)cat) {
        case MallCategory::剧情道具: price = 2000; break;
        case MallCategory::兵器:   price =  80; break;
        case MallCategory::护甲:   price =  80; break;
        case MallCategory::丹药:   price =  30; break;
        case MallCategory::暗器:   price =  60; break;
        case MallCategory::拳经:   price = 150; break;
        case MallCategory::剑谱:   price = 150; break;
        case MallCategory::刀录:   price = 150; break;
        case MallCategory::奇门:   price = 200; break;
        case MallCategory::心法:   price = 250; break;
        default: price = 50;
    }

    price += std::abs(item->AddAttack) * 12;
    price += std::abs(item->AddDefence) * 12;
    price += std::abs(item->AddHP) * 2;
    price += std::abs(item->AddMaxHP) * 5;
    price += std::abs(item->AddMP) * 3;
    price += std::abs(item->AddMaxMP) * 8;
    price += std::abs(item->AddSpeed) * 18;
    price += std::abs(item->AddMedicine) * 15;
    price += std::abs(item->AddUsePoison) * 15;
    price += std::abs(item->AddDetoxification) * 15;
    price += std::abs(item->AddAntiPoison) * 15;
    price += std::abs(item->AddFist) * 25;
    price += std::abs(item->AddSword) * 25;
    price += std::abs(item->AddKnife) * 25;
    price += std::abs(item->AddUnusual) * 25;
    price += std::abs(item->AddHiddenWeapon) * 25;
    price += std::abs(item->AddKnowledge) * 30;

    int rarity = std::abs(item->AddAttack) + std::abs(item->AddDefence)
              + (std::abs(item->AddHP) + std::abs(item->AddMaxHP)) / 5
              + std::abs(item->AddMedicine) + std::abs(item->AddFist);
    if (rarity > 30)  price = (int)(price * 1.5);
    if (rarity > 60)  price = price * 2;
    if (rarity > 100) price = price * 3;

    return std::max(1, price);
}

void UIMall::refreshCategoryItems() {
    category_items_ = getItemsByCategory(current_category_);
    selected_index_ = 0;
    scroll_offset_ = 0;
    buy_count_ = 1;
}

// ===================== 绘制 =====================

void UIMall::draw() {
    int x = x_, y = y_;
    auto* engine = Engine::getInstance();
    auto* font = Font::getInstance();
    auto* save = Save::getInstance();

    engine->fillColor({0, 0, 0, 160}, x, y, 800, 560);
    font->draw("商城", 28, x + 10, y + 6, {255, 220, 80, 255});
    drawCategoryTabs(x + 10, y + 36);
    drawItemList(x + 10, y + 70);
    drawItemDetail(x + 320, y + 70);
    drawBuyPanel(x + 10, y + 470);
    drawHint();  // 绘制提示信息（购买成功/失败等）

    int money = save->getMoneyCountInBag();
    std::string money_str = std::format("持有銀兩：{:8}", money);
    font->draw(money_str, 20, x + 500, y + 6, {255, 255, 100, 255});
}

void UIMall::drawCategoryTabs(int x, int y) {
    auto* font = Font::getInstance();
    int tab_w = 72;
    for (int i = 0; i < (int)MallCategory::COUNT; i++) {
        Color c = (i == current_category_) ? Color{255, 220, 80, 255} : Color{180, 180, 180, 255};
        if (i == current_category_) {
            Engine::getInstance()->fillColor({60, 50, 20, 200}, x + i * tab_w, y, tab_w - 4, 26);
        }
        font->draw(getCategoryName(i), 16, x + i * tab_w + 2, y + 4, c);
    }
}

void UIMall::drawItemList(int x, int y) {
    auto* font = Font::getInstance();
    int visible = 12;
    for (int i = 0; i < visible; i++) {
        int idx = scroll_offset_ + i;
        if (idx >= (int)category_items_.size()) break;
        Item* item = category_items_[idx];
        int price = calculateItemPrice(item);

        Color c = (idx == selected_index_) ? Color{255, 255, 100, 255} : Color{220, 220, 220, 255};
        if (idx == selected_index_) {
            Engine::getInstance()->fillColor({50, 45, 20, 180}, x, y + i * 28, 290, 26);
        }

        std::string name = item->Name;
        if (name.size() > 12) name = name.substr(0, 12);
        std::string line = std::format("{:18}{:8}兩", name, price > 0 ? std::to_string(price) : "不可");
        font->draw(line, 20, x + 4, y + i * 28 + 4, c);
    }

    if (category_items_.size() > visible) {
        std::string scroll_hint = std::format("↑↓ {}/{}", selected_index_ + 1, category_items_.size());
        font->draw(scroll_hint, 16, x + 220, y - 24, {150, 150, 150, 200});
    }
}

void UIMall::drawItemDetail(int x, int y) {
    auto* font = Font::getInstance();
    if (selected_index_ < 0 || selected_index_ >= (int)category_items_.size()) return;

    Item* item = category_items_[selected_index_];
    std::string name = item->Name;
    int price = calculateItemPrice(item);

    font->draw("名稱：" + name, 20, x, y, {255, 240, 180, 255});
    if (price <= 0) {
        font->draw("價格：不可購買", 20, x, y + 30, {255, 80, 80, 255});
    } else {
        font->draw(std::format("價格：{} 兩", price), 20, x, y + 30, {255, 220, 80, 255});
    }

    int dy = 60;
    auto draw_attr = [&](const std::string& label, int val, int dy_offset) {
        if (val != 0) {
            std::string sign = val > 0 ? "＋" : "﹣";
            font->draw(label + sign + std::to_string(std::abs(val)), 18, x, y + dy_offset, {180, 255, 180, 255});
            return dy_offset + 22;
        }
        return dy_offset;
    };
    dy = draw_attr("攻擊：", item->AddAttack, dy);
    dy = draw_attr("防禦：", item->AddDefence, dy);
    dy = draw_attr("生命：", item->AddHP, dy);
    dy = draw_attr("內力：", item->AddMP, dy);
    dy = draw_attr("速度：", item->AddSpeed, dy);
    dy = draw_attr("醫療：", item->AddMedicine, dy);
    dy = draw_attr("用毒：", item->AddUsePoison, dy);
    dy = draw_attr("解毒：", item->AddDetoxification, dy);
    dy = draw_attr("拳法：", item->AddFist, dy);
    dy = draw_attr("劍法：", item->AddSword, dy);
    dy = draw_attr("刀法：", item->AddKnife, dy);

    if (!item->Introduction.empty()) {
        std::string intro = item->Introduction;
        if (intro.size() > 40) intro = intro.substr(0, 40);
        font->draw("說明：" + intro, 16, x, y + 320, {200, 200, 200, 255});
    }
}

void UIMall::drawBuyPanel(int x, int y) {
    auto* font = Font::getInstance();
    font->draw(std::format("購買數量：{}", buy_count_), 20, x, y, {255, 255, 255, 255});

    if (selected_index_ >= 0 && selected_index_ < (int)category_items_.size()) {
        Item* item = category_items_[selected_index_];
        int price = calculateItemPrice(item);
        if (price > 0) {
            int total = price * buy_count_;
            font->draw(std::format("合計：{} 兩", total), 20, x + 200, y, {255, 220, 80, 255});

            int money = Save::getInstance()->getMoneyCountInBag();
            Color c = (money >= total) ? Color{100, 255, 100, 255} : Color{255, 80, 80, 255};
            font->draw(std::format("餘額：{} 兩", money - total), 20, x + 420, y, c);
        }
    }
}

// ===================== 事件處理 =====================

void UIMall::dealEvent(EngineEvent& e) {
    if (e.type == EVENT_KEY_UP) {
        // 分类切换
        if (e.key.key == K_LEFT && current_category_ > 0) {
            current_category_--;
            refreshCategoryItems();
        }
        if (e.key.key == K_RIGHT && current_category_ < (int)MallCategory::COUNT - 1) {
            current_category_++;
            refreshCategoryItems();
        }
        // 物品選擇
        if (e.key.key == K_UP && selected_index_ > 0) {
            selected_index_--;
            if (selected_index_ < scroll_offset_) scroll_offset_ = selected_index_;
        }
        if (e.key.key == K_DOWN && selected_index_ < (int)category_items_.size() - 1) {
            selected_index_++;
            if (selected_index_ >= scroll_offset_ + 12) scroll_offset_ = selected_index_ - 11;
        }
        // 購買數量
        if (e.key.key == K_PLUS || e.key.key == K_EQUALS) {
            buy_count_++;
        }
        if ((e.key.key == K_MINUS || e.key.key == K_MINUS) && buy_count_ > 1) {
            buy_count_--;
        }
    }

    // 按鈕事件
    if (btn_buy_->getState() == NodePress || btn_ok_->getState() == NodePress) {
        onPressedOK();
        btn_buy_->setState(NodeNormal);
        btn_ok_->setState(NodeNormal);
    }
    if (btn_cancel_->getState() == NodePress) {
        onPressedCancel();
        btn_cancel_->setState(NodeNormal);
    }
    if (btn_plus_->getState() == NodePress) {
        buy_count_++;
        btn_plus_->setState(NodeNormal);
    }
    if (btn_minus_->getState() == NodePress && buy_count_ > 1) {
        buy_count_--;
        btn_minus_->setState(NodeNormal);
    }
}

void UIMall::backRun() {
    // 提示计时器递减
    if (hint_timer_ > 0) {
        hint_timer_--;
        if (hint_timer_ == 0) {
            hint_text_.clear();
        }
    }
}

void UIMall::onPressedOK() {
    if (selected_index_ < 0 || selected_index_ >= (int)category_items_.size()) return;
    Item* item = category_items_[selected_index_];
    int price = calculateItemPrice(item);
    if (price <= 0) {
        showHint("此物品不可購買", {255, 150, 50, 255}, 1);
        return;
    }

    int total = price * buy_count_;
    int money = Save::getInstance()->getMoneyCountInBag();
    if (money < total) {
        showHint("銀兩不足，無法購買！", {255, 80, 80, 255}, 1);
        return;
    }

    // 扣除銀兩
    ::Event::getInstance()->addItemWithoutHint(Item::MoneyItemID, -total);
    // 獲得物品
    ::Event::getInstance()->addItemWithoutHint(item->ID, buy_count_);

    std::string msg = std::format("購買成功！獲得 {} x{}", PotConv::cp936toutf8(item->Name), buy_count_);
    showHint(msg, {100, 255, 100, 255}, 0);  // 绿色提示 + 成功音效

    refreshCategoryItems();
}

void UIMall::onPressedCancel() {
    setExit(true);
}

void UIMall::showHint(const std::string& text, Color color, int sound_id) {
    hint_text_ = text;
    hint_color_ = color;
    hint_timer_ = HINT_DURATION;
    if (sound_id >= 0) {
        Audio::getInstance()->playESound(sound_id);
    }
}

void UIMall::drawHint() {
    if (hint_timer_ <= 0 || hint_text_.empty()) return;

    auto* font = Font::getInstance();
    int x = x_ + 400, y = y_ + 280;  // 居中显示

    // 半透明背景
    int text_w = Font::getTextDrawSize(hint_text_) * 20 / 2 + 40;
    Engine::getInstance()->fillColor({0, 0, 0, 180}, x - text_w / 2, y - 10, text_w, 40);
    // 边框
    Engine::getInstance()->fillColor(hint_color_, x - text_w / 2, y - 10, text_w, 2);
    Engine::getInstance()->fillColor(hint_color_, x - text_w / 2, y + 28, text_w, 2);
    Engine::getInstance()->fillColor(hint_color_, x - text_w / 2, y - 10, 2, 40);
    Engine::getInstance()->fillColor(hint_color_, x + text_w / 2 - 2, y - 10, 2, 40);

    // 文字（带淡出效果）
    uint8_t alpha = (hint_timer_ > 30) ? 255 : (uint8_t)(hint_timer_ * 255 / 30);
    Color c = { (uint8_t)hint_color_.r, (uint8_t)hint_color_.g, (uint8_t)hint_color_.b, alpha };
    font->draw(hint_text_, 22, x - text_w / 2 + 20, y, c);
}
