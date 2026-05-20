#pragma once
#include "RunNode.h"
#include <vector>
#include <string>

extern bool g_talk_auto_play;
extern int g_talk_auto_play_delay_ms;

class Talk : public RunNode
{
public:
    Talk() {}
    Talk(std::string c, int h = -1) : Talk() { setContent(c); setHeadID(h); }
    virtual ~Talk() {}

    virtual void draw() override;
    virtual void dealEvent(EngineEvent& e) override;
    virtual void onPressedOK() override;
    void onSkip();

private:
    std::string content_;
    int head_id_ = -1;
    int head_style_ = 0;
    int current_line_ = 0;
    int width_ = 40;
    const int height_ = 4;    //每页行数
    std::vector<std::string> content_lines_;

    bool auto_playing_ = false;
    int64_t auto_play_prev_ticks_ = 0;

public:
    void setContent(std::string c) { content_ = c; }
    void setHeadID(int h) { head_id_ = h; }
    void setHeadStyle(int s) { head_style_ = s; }
    virtual void onEntrance() override;

    DEFAULT_CANCEL_EXIT;
};

