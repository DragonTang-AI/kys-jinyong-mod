#pragma once
#include "Menu.h"
#include <map>
#include <string>

class UISave : public MenuText
{
public:
    UISave();
    ~UISave();
private:
    enum
    {
        AUTO_SAVE_ID = 21,      // 自动档 ID
        MAX_SAVE_SLOT = 20,    // 最大存档数
    };

    int mode_ = 0;  //0为读档，1为存档
public:
    void setMode(int m) { mode_ = m; }

    void onEntrance() override;
    virtual void onPressedOK() override;

    static bool load(int r);
    static void save(int r);
    static void autoSave() { save(AUTO_SAVE_ID); }
};
