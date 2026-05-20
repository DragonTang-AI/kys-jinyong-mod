#include "Talk.h"
#include "Save.h"
extern bool g_talk_auto_play;
extern int g_talk_auto_play_delay_ms;  // 毫秒

void Talk::dealEvent(EngineEvent& e)
{
    // S 键：跳过当前对话
    if (e.type == EVENT_KEY_DOWN && e.key.key == K_S)
    {
        onSkip();
        return;
    }

    // 自动播放
    if (g_talk_auto_play)
    {
        int total_lines = (int)content_lines_.size();
        if (total_lines <= 0) { return; }

        if (!auto_playing_)
        {
            auto_playing_ = true;
            auto_play_prev_ticks_ = (int64_t)Engine::getTicks();
        }
        else
        {
            int64_t now = (int64_t)Engine::getTicks();
            if (now - auto_play_prev_ticks_ >= g_talk_auto_play_delay_ms)
            {
                auto_play_prev_ticks_ = now;
                if (current_line_ + height_ >= total_lines)
                {
                    setExit(true);
                }
                else
                {
                    current_line_ += height_;
                }
            }
        }
    }
    else
    {
        auto_playing_ = false;
    }
}

void Talk::onSkip()
{
    if (current_line_ + height_ >= (int)content_lines_.size())
    {
        setExit(true);  // 已经是最后页，直接退出
    }
    else
    {
        current_line_ = (int)content_lines_.size() - height_;  // 跳到倒数第二页（会再按一次退出）
    }
}

#include "Font.h"
#include "TextureManager.h"
#include <string>

void Talk::draw()
{
    int w = Engine::getInstance()->getUIWidth();
    int h = Engine::getInstance()->getUIHeight();

    if (!content_.empty())
    {
        x_ = 250;
        int x_text = x_ + 25;
        if (head_id_ >= 0)
        {
            if (head_style_ == 0)
            {
                x_ = 250;    // Adjust x_ for left-aligned head
                TextureManager::getInstance()->renderTexture("head", head_id_, x_ - 200, y_ + 50);
                x_text = x_ + 25;
            }
            else
            {
                x_ = w - 530 - 250;    // Adjust x_ for right-aligned head
                x_text = x_ + 25;
                TextureManager::getInstance()->renderTexture("head", head_id_, x_ + 530 + 50, y_ + 50);
            }
        }
        Engine::getInstance()->fillColor({ 0, 0, 0, 160 }, x_, y_ + 65, 530, 150);
        
        // 根据 head_id_ 画名字
        if (head_id_ >= 0) {
            auto role = Save::getInstance()->getRole(head_id_);
            if (role) {
                Font::getInstance()->draw(role->Name, 20, x_text, y_ + 68, { 255, 255, 255, 255 });
            }
        }
        
        int end_line = current_line_ + height_;
        if (end_line > content_lines_.size()) { end_line = content_lines_.size(); }
        for (int i = current_line_; i < end_line; i++)
        {
            Font::getInstance()->draw(content_lines_[i], 24, x_text, y_ + 75 + 34 * (i - current_line_), { 255, 255, 255, 255 });
        }
    }
}

void Talk::onPressedOK()
{
    Engine::getInstance()->setInterValControllerPress(200);
    if (current_line_ + height_ >= content_lines_.size())
    {
        setExit(true);
    }
    else
    {
        current_line_ += height_;
    }
    //e.type = BP_FIRSTEVENT;
    result_ = 0;
}

void Talk::onEntrance()
{
    int w = Engine::getInstance()->getPresentWidth();
    //width_ = (w - 400) / 12;    // 每行宽度
    content_lines_.clear();
    current_line_ = 0;
    int i = 0;
    while (i < content_.size())
    {
        int len = 0;
        //计算英文个数
        int eng_count = 0;
        int j = i;
        for (; j < content_.size();)
        {
            if (uint8_t(content_.at(j)) > 128)
            {
                j += 3;
                len += 2;
            }
            else
            {
                eng_count++;
                j++;
                len += 1;
            }
            if (len >= width_)
            {
                break;
            }
        }
        auto line = content_.substr(i, j - i);
        content_lines_.push_back(line);
        i = j;
    }
}