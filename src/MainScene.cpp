#include "MainScene.h"
#include "BattleMap.h"
#include "Console.h"
#include "Event.h"
#include "Fade.h"
#include "Font.h"
#include "GameUtil.h"
#include "GrpIdxFile.h"
#include "Random.h"
#include "Save.h"
#include "SubScene.h"
#include "TextureManager.h"
#include "TextBox.h"
#include "UI.h"
#include "UISave.h"
#include "UIGameAssist.h"
#include "Weather.h"
#include <algorithm>
#include <vector>
#include <SDL3/SDL.h>

namespace
{

int g_mainmap_encounter_cd = 0;

bool battleRowHasEnemy(int battle_index)
{
    auto* inf = BattleMap::getInstance()->getBattleInfo(battle_index);
    if (!inf)
    {
        return false;
    }
    for (int i = 0; i < BATTLE_ENEMY_COUNT; i++)
    {
        if (inf->Enemy[i] > 0)
        {
            return true;
        }
    }
    return false;
}

void clampRangeToBattles(int& lo, int& hi, int count)
{
    if (count <= 0)
    {
        lo = hi = 0;
        return;
    }
    lo = std::max(0, std::min(lo, count - 1));
    hi = std::max(0, std::min(hi, count - 1));
    if (hi < lo)
    {
        std::swap(lo, hi);
    }
}

int randomBattleFromTier(int tier, int battle_count);

int randomBattleFromTier(int tier, int battle_count)
{
    auto* gu = GameUtil::getInstance();
    int lo = gu->getInt("mainmap_encounter", "tier0_battle_min", 0);
    int hi = gu->getInt("mainmap_encounter", "tier0_battle_max", 32);
    if (tier == 1)
    {
        lo = gu->getInt("mainmap_encounter", "tier1_battle_min", 33);
        hi = gu->getInt("mainmap_encounter", "tier1_battle_max", 65);
    }
    else if (tier == 2)
    {
        lo = gu->getInt("mainmap_encounter", "tier2_battle_min", 66);
        hi = gu->getInt("mainmap_encounter", "tier2_battle_max", 100);
    }
    else if (tier >= 3)
    {
        lo = gu->getInt("mainmap_encounter", "tier3_battle_min", 101);
        hi = gu->getInt("mainmap_encounter", "tier3_battle_max", 139);
    }
    clampRangeToBattles(lo, hi, battle_count);
    RandomDouble r;
    for (int attempt = 0; attempt < 24; attempt++)
    {
        int pick = lo + r.rand_int(hi - lo + 1);
        if (battleRowHasEnemy(pick))
        {
            return pick;
        }
    }
    return -1;
}

int regionTierFromSumXY(int sumxy)
{
    auto* gu = GameUtil::getInstance();
    int t1 = gu->getInt("mainmap_encounter", "tier1_max_sumxy", 260);
    int t2 = gu->getInt("mainmap_encounter", "tier2_max_sumxy", 460);
    int t3 = gu->getInt("mainmap_encounter", "tier3_max_sumxy", 700);
    if (sumxy <= t1)
    {
        return 0;
    }
    if (sumxy <= t2)
    {
        return 1;
    }
    if (sumxy <= t3)
    {
        return 2;
    }
    return 3;
}


void maybeMainMapRandomEncounter(MainScene* scene, int tile_x, int tile_y)
{
    if (!scene)
    {
        return;
    }
    auto* gu = GameUtil::getInstance();
    if (!gu->getInt("mainmap_encounter", "enable", 1))
    {
        return;
    }
    if (g_mainmap_encounter_cd > 0)
    {
        return;
    }
    int battle_count = BattleMap::getInstance()->getBattleInfoCount();
    if (battle_count <= 0)
    {
        return;
    }
    int tier = regionTierFromSumXY(tile_x + tile_y);
    int base = gu->getInt("mainmap_encounter", "chance_per_10000", 260);
    int extra = gu->getInt("mainmap_encounter", "chance_extra_per_tier", 80);
    int chance = std::min(10000, base + tier * extra);
    RandomDouble r;
    if (r.rand_int(10000) >= chance)
    {
        return;
    }
    int bid = randomBattleFromTier(tier, battle_count);
    if (bid < 0)
    {
        return;
    }
    if (gu->getInt("mainmap_encounter", "show_notice", 1))
    {
        std::string msg = gu->getString("mainmap_encounter", "notice_text", "野外遭遇敵人！");
        auto notice = std::make_shared<TextBox>();
        notice->setText(std::move(msg));
        notice->setFontSize(gu->getInt("mainmap_encounter", "notice_font_size", 22));
        int w = Engine::getInstance()->getUIWidth();
        int h = Engine::getInstance()->getUIHeight();
        notice->setTextPosition(24, 8);
        if (notice->runAtPosition(std::max(40, w / 2 - 180), std::max(80, h / 2 - 40)) != 0)
        {
            if (gu->getInt("mainmap_encounter", "notice_cancel_cooldown", 0))
            {
                g_mainmap_encounter_cd = std::max(0, gu->getInt("mainmap_encounter", "cooldown_steps", 12));
            }
            return;
        }
    }
    Event::getInstance()->tryBattle(bid, 0);
    g_mainmap_encounter_cd = std::max(0, gu->getInt("mainmap_encounter", "cooldown_steps", 12));
}

}    // namespace

MainScene::MainScene()
{
    full_window_ = 1;
    COORD_COUNT = MAINMAP_COORD_COUNT;

    if (!data_readed_)
    {
        MapSquareInt earth_layer1(COORD_COUNT), surface_layer1(COORD_COUNT), building_layer1(COORD_COUNT);

        earth_layer_.resize(COORD_COUNT);
        surface_layer_.resize(COORD_COUNT);
        building_layer_.resize(COORD_COUNT);
        build_x_layer_.resize(COORD_COUNT);
        build_y_layer_.resize(COORD_COUNT);

        int length = COORD_COUNT * COORD_COUNT * sizeof(MAP_INT);

        GrpIdxFile::readFile(GameUtil::PATH() + "resource/earth.002", &earth_layer1.data(0), length);
        GrpIdxFile::readFile(GameUtil::PATH() + "resource/surface.002", &surface_layer1.data(0), length);
        GrpIdxFile::readFile(GameUtil::PATH() + "resource/building.002", &building_layer1.data(0), length);
        GrpIdxFile::readFile(GameUtil::PATH() + "resource/buildx.002", &build_x_layer_.data(0), length);
        GrpIdxFile::readFile(GameUtil::PATH() + "resource/buildy.002", &build_y_layer_.data(0), length);

        divide2(earth_layer1, earth_layer_);
        divide2(surface_layer1, surface_layer_);
        divide2(building_layer1, building_layer_);
    }
    data_readed_ = true;

    cloud_group_ = std::make_shared<CloudGroup>();
    cloud_group_->init(100, COORD_COUNT * TILE_W * 2, COORD_COUNT * TILE_H * 2);
    addChild(cloud_group_);
    //getEntrance();
    addChild(Weather::getInstance());
}

MainScene::~MainScene()
{
}

void MainScene::divide2(MapSquareInt& m1, MapSquare<Object>& m)
{
    for (int i = 0; i < m1.squareSize(); i++)
    {
        m1.data(i) /= 2;
        if (m1.data(i) > 0)
        {
            m.data(i).tex_ = TextureManager::getInstance()->getTexture("mmap", m1.data(i));
            auto pic = m1.data(i);
            auto& a = m.data(i);
            if (pic == 419 || pic >= 306 && pic <= 335)
            {
                a.material_ = ObjectMaterial::Water;
                a.can_walk_ = 0;
            }
            else if (pic >= 179 && pic <= 181 || pic >= 253 && pic <= 335 || pic >= 508 && pic <= 511)
            {
                a.material_ = ObjectMaterial::Water;
                a.can_walk_ = 1;
            }
            else if (pic >= 1008 && pic <= 1164 || pic >= 1214 && pic <= 1238)
            {
                a.material_ = ObjectMaterial::Wood;
            }
        }
        else
        {
            m.data(i).tex_ = nullptr;
        }
    }
}

void MainScene::draw()
{
    Engine::getInstance()->setRenderTarget("scene");

    struct DrawInfo
    {
        int index;
        TextureWarpper* tex;
        Point p;
    };

    //Timer t1;
    //std::map<int, DrawInfo> map;
    static std::vector<DrawInfo> building_vec(10000);
    int building_count = 0;
    //TextureManager::getInstance()->renderTexture("mmap", 0, 0, 0);
    Engine::getInstance()->fillColor({ 0, 0, 0, 255 }, 0, 0, -1, -1);

    //新的画地面方法
    if (TextureManager::getInstance()->getTextureGroup("mmap-earth")->getTextureCount() > 0)
    {
        const int earth_size = TILE_W * 480 * 2;
        auto pe = getPositionOnRender(0, 0, man_x_, man_y_);
        int earth_x = pe.x - earth_size / 2;
        int earth_y = pe.y - TILE_H * 2 + 1;
        int view_w, view_h;
        Engine::getInstance()->getAssistTextureSize("scene", view_w, view_h);
        Engine::getInstance()->createAssistTexture("temp", view_w, view_h);
        Engine::getInstance()->setRenderTarget("temp");
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                int x = i * earth_size / 8 + earth_x;
                int y = j * earth_size / 8 / 2 + earth_y;
                int w = earth_size / 8;
                int h = earth_size / 8 / 2;
                if (x > view_w || y > view_h || x + w < 0 || y + h < 0)
                {
                    continue;
                }
                TextureManager::getInstance()->renderTexture("mmap-earth", i + j * 8, i * earth_size / 8 + earth_x, j * earth_size / 8 / 2 + earth_y);
            }
        }
        Engine::getInstance()->setRenderTarget("scene");
        std::vector<float> brightness_v(4, 0);
        brightness_v[0] = 0.5;
        brightness_v[2] = 0;
        std::vector<Color> color_v(4, { 255, 255, 255, 255 });
        auto temp_texture = Engine::getInstance()->getTexture("temp");
        Rect r = { 0, 0, view_w, view_h };
        Engine::getInstance()->renderTextureLight(temp_texture, nullptr, &r, color_v, brightness_v);
    }

    //下面的15是下方较高贴图的余量，其余场景同
    for (int sum = -view_sum_region_; sum <= view_sum_region_ + 15; sum++)
    {
        for (int i = -view_width_region_; i <= view_width_region_; i++)
        {
            int ix = man_x_ + i + (sum / 2);
            int iy = man_y_ - i + (sum - sum / 2);
            auto p = getPositionOnRender(ix, iy, man_x_, man_y_);
            p.x += x_;
            p.y += y_;
            //auto p = getMapPoint(ix, iy, *_Mx, *_My);
            if (!isOutLine(ix, iy))
            {
                //共分3层，地面，表面，建筑，主角包括在建筑中
                if (TextureManager::getInstance()->getTextureGroup("mmap-earth")->getTextureCount() == 0)
                {
                    //调试模式下不画出地面，图的数量太多占用CPU很大--取消
                    if (earth_layer_.data(ix, iy).getTexture())
                    {
                        TextureManager::getInstance()->renderTexture(earth_layer_.data(ix, iy).getTexture(), p.x, p.y);
                    }
                    if (surface_layer_.data(ix, iy).getTexture())
                    {
                        TextureManager::getInstance()->renderTexture(surface_layer_.data(ix, iy).getTexture(), p.x, p.y);
                    }
                }
                if (building_layer_.data(ix, iy).getTexture())
                {
                    //根据图片的宽度计算图的中点, 为避免出现小数, 实际是中点坐标的2倍
                    //次要排序依据是y坐标
                    //直接设置z轴
                    auto tex = building_layer_.data(ix, iy).getTexture();
                    auto w = tex->w;
                    auto h = tex->h;
                    auto dy = tex->dy;
                    int c = ((ix + iy) - (w + TILE_W * 2 - 1) / (TILE_W * 2) - (dy - h + 1) / (TILE_H / 2)) * 1024 + ix;
                    //map[2 * c + 1] = { 2*c+1, t, p };
                    building_vec[building_count++] = { 2 * c + 1, tex, p };
                }
                if (ix == man_x_ && iy == man_y_)
                {
                    if (isWater(man_x_, man_y_))
                    {
                        man_pic_ = SHIP_PIC_0 + Scene::towards_ * SHIP_PIC_COUNT + step_;
                    }
                    else
                    {
                        man_pic_ = MAN_PIC_0 + Scene::towards_ * MAN_PIC_COUNT + step_;    //每个方向的第一张是静止图
                        if (rest_time_ >= BEGIN_REST_TIME)
                        {
                            man_pic_ = REST_PIC_0 + Scene::towards_ * REST_PIC_COUNT + (rest_time_ - BEGIN_REST_TIME) / REST_INTERVAL % REST_PIC_COUNT;
                        }
                    }
                    int c = 1024 * (ix + iy) + ix;
                    //map[2 * c] = {2*c, man_pic_, p };
                    building_vec[building_count++] = { 2 * c, TextureManager::getInstance()->getTexture("mmap", man_pic_), p };
                }
            }
        }
    }
    //for (auto i = map.begin(); i != map.end(); i++)
    //{
    //    TextureManager::getInstance()->renderTexture("mmap", i->second.i, i->second.p.x, i->second.p.y);
    //}

    auto sort_building = [](DrawInfo& d1, DrawInfo& d2)
    {
        return d1.index < d2.index;
    };
    std::sort(building_vec.begin(), building_vec.begin() + building_count, sort_building);
    for (int i = 0; i < building_count; i++)
    {
        auto& d = building_vec[i];
        std::vector<float> brightness_v(4, 0);
        brightness_v[0] = 1;
        brightness_v[2] = 0;
        TextureManager::getInstance()->renderTexture(d.tex, d.p.x, d.p.y,
            { { 255, 255, 255, 255 }, 255, 1, 1, 0, 0, {}, brightness_v });
    }
    auto p = getPositionOnRender(cursor_x_, cursor_y_, man_x_, man_y_);
    TextureManager::getInstance()->renderTexture("mmap", 1, p.x, p.y, { { 255, 255, 255, 255 }, 128 });

    //LOG("%d buildings in %g s.\n", building_count, t1.getElapsedTime());
    //Engine::getInstance()->setColor(Engine::getInstance()->getTexture(), { 227, 207, 87, 255 });
    Engine::getInstance()->renderTextureToMain("scene");
    UIGameAssist::applyActiveCheats();
    drawPositionInfo();

    // 世界地图
    if (show_worldmap_)
    {
        drawWorldMap();
    }
}

void MainScene::drawPositionInfo()
{
    auto text = std::format("大地图 ({}, {})", man_x_, man_y_);
    Font::getInstance()->draw(text, 20, Engine::getInstance()->getUIWidth() - 320, 15, { 255, 255, 0, 255 });
}

void MainScene::backRun()
{
    rest_time_++;    //只要出现走动，rest_time就会清零
    //云的贴图
    view_cloud_ = cloud_group_->setPositionOnScreen(man_x_, man_y_, render_center_x_, render_center_y_);

    Weather::getInstance()->setWeather(inNorth(), view_cloud_);
}

void MainScene::dealEvent(EngineEvent& e)
{
    // 世界地图模式：拦截所有事件
    if (show_worldmap_)
    {
        dealWorldMapEvent(e);
        return;
    }

    auto engine = Engine::getInstance();
    //强制进入，通常用于开始
    if (force_submap_ >= 0)
    {
        setVisible(true);
        auto sub_map = std::make_shared<SubScene>(force_submap_);
        sub_map->setManViewPosition(force_submap_x_, force_submap_y_);
        sub_map->setTowards(towards_);
        sub_map->setForceBeginEvent(force_event_);
        sub_map->run();
        towards_ = sub_map->towards_;
        //Fade::fadeIn(8);
        force_submap_ = -1;
        force_event_ = -1;
    }

    // Tab激活控制台
    if (e.type == EVENT_KEY_UP && e.key.key == K_TAB
        || (e.type == EVENT_GAMEPAD_BUTTON_UP && e.gbutton.button == GAMEPAD_BUTTON_BACK))
    {
        Console c;
    }
    // M键切换世界地图
    if (e.type == EVENT_KEY_UP && e.key.key == SDLK_M)
    {
        show_worldmap_ = !show_worldmap_;
        return;
    }
    if ((e.type == EVENT_KEY_UP && e.key.key == K_ESCAPE)
        || (e.type == EVENT_MOUSE_BUTTON_UP && e.button.button == BUTTON_RIGHT)
        //|| (e.type == EVENT_GAMEPAD_BUTTON_UP && e.gbutton.button == GAMEPAD_BUTTON_START)
        || engine->gameControllerGetButton(GAMEPAD_BUTTON_START))
    {
        UI::getInstance()->run();
    }
    //LOG("{} {} {}\n",current_frame_, Engine::getTicks(), Timer::getNowAsString());
    int x = man_x_, y = man_y_;
    if (engine->getTicks() - pre_pressed_ticks_ > key_walk_delay_)
    {
        //键盘走路部分，检测4个方向键
        int pressed = 0;
        pre_pressed_ticks_ = engine->getTicks();
        auto axis_x = engine->gameControllerGetAxis(GAMEPAD_AXIS_LEFTX);
        auto axis_y = engine->gameControllerGetAxis(GAMEPAD_AXIS_LEFTY);
        if (abs(axis_x) < 10000) { axis_x = 0; }
        if (abs(axis_y) < 10000) { axis_y = 0; }
        if (axis_x != 0 || axis_y != 0)
        {
            Pointf axis{ float(axis_x), float(axis_y) };
            auto to = realTowardsToFaceTowards(axis);
            if (to == Towards_LeftUp) { pressed = K_LEFT; }
            if (to == Towards_LeftDown) { pressed = K_DOWN; }
            if (to == Towards_RightDown) { pressed = K_RIGHT; }
            if (to == Towards_RightUp) { pressed = K_UP; }
        }
        if (engine->gameControllerGetButton(GAMEPAD_BUTTON_DPAD_LEFT)) { pressed = K_LEFT; }
        if (engine->gameControllerGetButton(GAMEPAD_BUTTON_DPAD_DOWN)) { pressed = K_DOWN; }
        if (engine->gameControllerGetButton(GAMEPAD_BUTTON_DPAD_RIGHT)) { pressed = K_RIGHT; }
        if (engine->gameControllerGetButton(GAMEPAD_BUTTON_DPAD_UP)) { pressed = K_UP; }
        if (engine->checkKeyPress(K_A)) { pressed = K_LEFT; }
        if (engine->checkKeyPress(K_S)) { pressed = K_DOWN; }
        if (engine->checkKeyPress(K_D)) { pressed = K_RIGHT; }
        if (engine->checkKeyPress(K_W)) { pressed = K_UP; }

        for (auto i = int(K_RIGHT); i <= int(K_UP); i++)
        {
            if (i != pre_pressed_ && engine->checkKeyPress(i))
            {
                pressed = i;
            }
        }
        if (pressed == 0 && engine->checkKeyPress(pre_pressed_))
        {
            pressed = pre_pressed_;
        }
        pre_pressed_ = pressed;

        if (pressed)
        {
            //注意，中间空出几个步数是为了可以单步行动，子场景同
            if (total_step_ < 1 || total_step_ >= first_step_delay_)
            {
                changeTowardsByKey(pressed);
                getTowardsPosition(man_x_, man_y_, towards_, &x, &y);
                if (tryWalk(x, y))
                {
                    if (g_mainmap_encounter_cd > 0)
                    {
                        g_mainmap_encounter_cd--;
                    }
                    if (!checkEntrance(x, y, true) && !isWater(x, y))
                    {
                        maybeMainMapRandomEncounter(this, x, y);
                    }
                }
            }
            total_step_++;
        }
        else
        {
            if (rest_time_ > 2) { total_step_ = 0; }    //虚拟按键中间可能出现空白帧，此处减少清空的情况，为了第一步可以不连续
        }

        if (pressed && checkEntrance(x, y))
        {
            way_que_.clear();
            total_step_ = 0;
        }

        if (!way_que_.empty())
        {
            Point p = way_que_.back();
            x = p.x;
            y = p.y;
            auto tw = calTowards(man_x_, man_y_, x, y);
            if (tw != Towards_None)
            {
                towards_ = tw;
            }
            if (tryWalk(x, y))
            {
                if (g_mainmap_encounter_cd > 0)
                {
                    g_mainmap_encounter_cd--;
                }
                if (!checkEntrance(x, y, true) && !isWater(x, y))
                {
                    maybeMainMapRandomEncounter(this, x, y);
                }
            }
            way_que_.pop_back();
            if (way_que_.empty() && mouse_event_x_ >= 0 && mouse_event_y_ >= 0)
            {
                towards_ = calTowards(man_x_, man_y_, mouse_event_x_, mouse_event_y_);
                if (checkEntrance(mouse_event_x_, mouse_event_y_))
                {
                    way_que_.clear();
                    setMouseEventPoint(-1, -1);
                }
            }
        }
    }
    calCursorPosition(man_x_, man_y_);

    //鼠标寻路
    if (e.type == EVENT_MOUSE_BUTTON_UP && e.button.button == BUTTON_LEFT)
    {
        setMouseEventPoint(-1, -1);
        int mx, my;
        Engine::getInstance()->getMouseStateInStartWindow(mx, my);
        Point p = getMousePosition(mx, my, x, y);
        way_que_.clear();
        if (canWalk(p.x, p.y) /* && !isOutScreen(p.x, p.y)*/)
        {
            FindWay(x, y, p.x, p.y);
        }
        //如果是建筑，在此建筑的附近试图查找入口
        if (isBuilding(p.x, p.y))
        {
            int buiding_x = build_x_layer_.data(p.x, p.y);
            int buiding_y = build_y_layer_.data(p.x, p.y);
            bool found_entrance = false;
            for (int ix = buiding_x + 1; ix > buiding_x - 9; ix--)
            {
                for (int iy = buiding_y + 1; iy > buiding_y - 9; iy--)
                {
                    if (build_x_layer_.data(ix, iy) == buiding_x && build_y_layer_.data(ix, iy) == buiding_y && checkEntrance(ix, iy, true))
                    {
                        p.x = ix;
                        p.y = iy;    //p的值变化了
                        found_entrance = true;
                        break;
                    }
                }
                if (found_entrance)
                {
                    break;
                }
            }
            if (found_entrance)
            {
                //在入口四周查找一个可以走到的地方
                std::vector<Point> ps;
                if (canWalk(p.x - 1, p.y))
                {
                    ps.push_back({ p.x - 1, p.y });
                }
                if (canWalk(p.x + 1, p.y))
                {
                    ps.push_back({ p.x + 1, p.y });
                }
                if (canWalk(p.x, p.y - 1))
                {
                    ps.push_back({ p.x, p.y - 1 });
                }
                if (canWalk(p.x, p.y + 1))
                {
                    ps.push_back({ p.x, p.y + 1 });
                }
                if (!ps.empty())
                {
                    RandomDouble r;
                    int i = r.rand_int(ps.size());
                    FindWay(x, y, ps[i].x, ps[i].y);
                    setMouseEventPoint(p.x, p.y);
                }
            }
        }
    }
}

void MainScene::onEntrance()
{
    calViewRegion();
    //if (force_submap_ >= 0)
    //{
    //    forceEnterSubScene(force_submap_, force_submap_x_, force_submap_y_);
    //}
    //一大块地面的纹理
    //earth_texture = Engine::getInstance()->createRenderedTexture(COORD_COUNT * TILE_W * 2, COORD_COUNT * TILE_H * 2);
}

void MainScene::onExit()
{
}

void MainScene::onPressedCancel()
{
}

bool MainScene::tryWalk(int x, int y)
{
    bool moved = canWalk(x, y);
    if (moved)
    {
        man_x_ = x;
        man_y_ = y;
    }
    step_++;
    if (isWater(man_x_, man_y_))
    {
        step_ = step_ % SHIP_PIC_COUNT;
    }
    else
    {
        if (step_ >= MAN_PIC_COUNT)
        {
            step_ = 1;
        }
    }
    rest_time_ = 0;
    return moved;
}

void MainScene::setEntrance()
{
}

bool MainScene::isBuilding(int x, int y)
{
    if (isOutLine(x, y))
    {
        return false;
    }
    return (building_layer_.data(build_x_layer_.data(x, y), build_y_layer_.data(x, y)).getTexture() != nullptr);
}

int MainScene::isWater(int x, int y)
{
    return earth_layer_.data(x, y).material_ == ObjectMaterial::Water;
}

bool MainScene::isOutScreen(int x, int y)
{
    return (abs(man_x_ - x) >= 2 * view_width_region_ || abs(man_y_ - y) >= view_sum_region_);
}

bool MainScene::canWalk(int x, int y)
{
    //这里不需要加，实际上入口都是无法走到的
    if (isOutLine(x, y) || isBuilding(x, y))    // || isWater(x, y))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool MainScene::checkEntrance(int x, int y, bool only_check /*= false*/)
{
    for (int i = 0; i < Save::getInstance()->getSubMapInfos().size(); i++)
    {
        auto s = Save::getInstance()->getSubMapInfo(i);
        if (x == s->MainEntranceX1 && y == s->MainEntranceY1 || x == s->MainEntranceX2 && y == s->MainEntranceY2)
        {
            bool can_enter = false;
            if (s->EntranceCondition == 0)
            {
                can_enter = true;
            }
            else if (s->EntranceCondition == 2)
            {
                //注意进入条件2的设定
                for (auto r : Save::getInstance()->Team)
                {
                    if (Save::getInstance()->getRole(r)->Speed >= 70)
                    {
                        can_enter = true;
                        break;
                    }
                }
            }
            if (only_check)
            {
                return true;
            }
            if (can_enter)
            {
                UISave::autoSave();
                //这里看起来要主动多画一帧，待修
                drawAndPresent();
                auto sub_map = std::make_shared<SubScene>(i);
                sub_map->setManViewPosition(s->EntranceX, s->EntranceY);
                sub_map->run();
                towards_ = sub_map->towards_;
                //Fade::fadeIn(8);
                return true;
            }
        }
    }
    return false;
}

void MainScene::forceEnterSubScene(int submap_id, int x, int y, int event)
{
    force_submap_ = submap_id;
    if (x >= 0)
    {
        force_submap_x_ = x;
    }
    if (y >= 0)
    {
        force_submap_y_ = y;
    }
    force_event_ = event;
    setVisible(false);
}

void MainScene::drawWorldMap()
{
    auto engine = Engine::getInstance();
    int screen_w = engine->getUIWidth();
    int screen_h = engine->getUIHeight();

    // 全屏半透明黑色背景
    engine->fillColor({ 0, 0, 0, 220 }, 0, 0, screen_w, screen_h);

    // 计算缩放：480x480 地图适配屏幕（留边距）
    int margin = 40;
    int map_display_size = std::min(screen_w - margin * 2, screen_h - margin * 2 - 30);
    float scale = (float)map_display_size / MAINMAP_COORD_COUNT;
    int offset_x = (screen_w - map_display_size) / 2;
    int offset_y = (screen_h - map_display_size) / 2 + 10;

    // 绘制地图边框
    engine->fillColor({ 20, 20, 20, 255 }, offset_x - 1, offset_y - 1, map_display_size + 2, map_display_size + 2);

    // 绘制地形（区分：水、草地、沙漠/土、雪山、建筑）
    for (int y = 0; y < MAINMAP_COORD_COUNT; y++)
    {
        for (int x = 0; x < MAINMAP_COORD_COUNT; x++)
        {
            if (x < 0 || x >= MAINMAP_COORD_COUNT || y < 0 || y >= MAINMAP_COORD_COUNT) { continue; }

            Color c;
            bool building = isBuilding(x, y);
            int water = isWater(x, y);

            if (building)
            {
                c = { 139, 90, 43, 255 };      // 建筑：棕色
            }
            else if (water)
            {
                c = { 30, 80, 180, 255 };      // 水：深蓝
            }
            else
            {
                // 通过 earth_layer_ 的纹理 pic ID 区分更多地形
                auto& obj = earth_layer_.data(x, y);
                int pic = -1;
                if (obj.tex_) {
                    // 从纹理反推 pic 范围来区分地形
                    auto mat = obj.material_;
                    if (mat == ObjectMaterial::Wood) {
                        c = { 34, 120, 34, 255 };   // 林地：亮绿
                    } else {
                        // 默认草地，根据位置微调色调模拟地形变化
                        // 北部偏冷（蓝绿），南部偏暖（黄绿），中部正常绿
                        int dist_from_center = abs(y - MAINMAP_COORD_COUNT / 2);
                        if (y < 100) {
                            c = { 200, 220, 240, 255 };  // 雪山：浅白蓝（北部区域）
                        } else if (y < 160) {
                            c = { 180, 190, 170, 255 };  // 山地：灰绿
                        } else if (y > 380) {
                            c = { 194, 178, 128, 255 };  // 沙漠/土：黄褐色（南部区域）
                        } else if (y > 320) {
                            c = { 160, 180, 80, 255 };   // 草原：黄绿
                        } else {
                            c = { 45, 110, 45, 255 };   // 普通：深绿
                        }
                    }
                }
                else
                {
                    c = { 45, 110, 45, 255 };       // 默认地面：深绿
                }
            }

            int px = offset_x + (int)(x * scale);
            int py = offset_y + (int)(y * scale);
            int pw = (int)((x + 1) * scale) - px;
            int ph = (int)((y + 1) * scale) - py;
            if (pw < 1) pw = 1;
            if (ph < 1) ph = 1;
            if (x < MAINMAP_COORD_COUNT - 1) pw += 1;
            if (y < MAINMAP_COORD_COUNT - 1) ph += 1;
            engine->fillColor(c, px, py, pw, ph);
        }
    }

    // 绘制入口标记 + 地名标注
    for (int i = 0; i < Save::getInstance()->getSubMapInfos().size(); i++)
    {
        auto s = Save::getInstance()->getSubMapInfo(i);
        if (s->EntranceCondition != 0 && s->EntranceCondition != 2) { continue; }

        auto drawEntranceMarker = [&](int ex, int ey) {
            if (ex <= 0 || ey <= 0) return;
            int px = offset_x + (int)(ex * scale);
            int py = offset_y + (int)(ey * scale);
            int sz = std::max(4, (int)(2.5 * scale));
            // 入口标记：青色圆点带白色边框
            engine->fillColor({ 0, 220, 220, 255 }, px - sz/2, py - sz/2, sz, sz);
            engine->fillColor({ 255, 255, 255, 200 }, px - sz/2, py - sz/2, sz, 1);
            engine->fillColor({ 255, 255, 255, 200 }, px - sz/2, py + sz/2 - 1, sz, 1);
            engine->fillColor({ 255, 255, 255, 200 }, px - sz/2, py - sz/2, 1, sz);
            engine->fillColor({ 255, 255, 255, 200 }, px + sz/2 - 1, py - sz/2, 1, sz);
        };
        drawEntranceMarker(s->MainEntranceX1, s->MainEntranceY1);
        drawEntranceMarker(s->MainEntranceX2, s->MainEntranceY2);

        // 地名标注（在第一个入口点上方显示名称）
        if (s->MainEntranceX1 > 0 && s->MainEntranceY1 > 0 && !s->Name.empty())
        {
            int name_px = offset_x + (int)(s->MainEntranceX1 * scale);
            int name_py = offset_y + (int)(s->MainEntranceY1 * scale) - std::max(6, (int)(3 * scale)) - 2;
            Font::getInstance()->draw(s->Name, 14, name_px, name_py, { 255, 255, 200, 230 });
        }
    }

    // 绘制主角位置（高德地图风格定位三角）
    int man_px = offset_x + (int)(man_x_ * scale);
    int man_py = offset_y + (int)(man_y_ * scale);
    int tri_size = std::max(8, (int)(5 * scale));  // 三角大小

    // 外圈光晕脉冲效果（简单的白色半透明圆）
    int pulse_r = tri_size + 3 + (SDL_GetTicks() % 1000) / 250;  // 3~6 像素脉动
    engine->fillColor({ 255, 50, 50, 60 }, man_px - pulse_r, man_py - pulse_r, pulse_r * 2, pulse_r * 2);

    // 定位三角（向下指的等腰三角形，类似高德/百度地图定位图标）
    // 三角顶点在底部中心，底边在上
    Color tri_color = { 255, 50, 50, 255 };     // 红色填充
    Color tri_border = { 255, 255, 255, 255 };  // 白色边框
    int cx = man_px;                           // 中心 x
    int top_y = man_py - tri_size;              // 三角顶部 y
    int bot_y = man_py + tri_size / 2;          // 三角底部 y
    int half_w = tri_size;                      // 半宽

    // 用 fillRect 近似画三角（实心三角形用多个水平线段）
    for (int dy = 0; dy <= bot_y - top_y; dy++)
    {
        float progress = (float)dy / (bot_y - top_y);  // 0=顶, 1=底
        int line_w;
        if (progress < 0.6f) {
            // 上半部分：从尖顶逐渐变宽
            line_w = (int)(half_w * progress / 0.6f);
        } else {
            // 下半部分：保持最宽然后收窄到底部尖端
            float lower_progress = (progress - 0.6f) / 0.4f;
            line_w = (int)(half_w * (1.0f - lower_progress * 0.8f));
        }
        if (line_w < 1) line_w = 1;
        int ly = top_y + dy;
        engine->fillColor(tri_color, cx - line_w, ly, line_w * 2, 1);
    }
    // 白色边框轮廓（简化：只画左右边缘和底部尖端）
    engine->fillColor(tri_border, cx - tri_size, top_y + 2, 1, bot_y - top_y - 3);
    engine->fillColor(tri_border, cx + tri_size - 1, top_y + 2, 1, bot_y - top_y - 3);
    engine->fillColor(tri_border, cx - 3, bot_y - 2, 7, 1);

    // 标题 + 坐标
    Font::getInstance()->draw("世界地图 - 点击寻路 (M关闭)", 22, offset_x, offset_y - 28, { 255, 255, 255, 255 });
    auto pos_text = std::format("位置: ({}, {})", man_x_, man_y_);
    Font::getInstance()->draw(pos_text, 18, offset_x + map_display_size - 200, offset_y - 25, { 255, 255, 0, 255 });
}

void MainScene::dealWorldMapEvent(EngineEvent& e)
{
    // M键或ESC关闭世界地图
    if ((e.type == EVENT_KEY_UP && (e.key.key == SDLK_M || e.key.key == K_ESCAPE))
        || (e.type == EVENT_MOUSE_BUTTON_UP && e.button.button == BUTTON_RIGHT))
    {
        show_worldmap_ = false;
        return;
    }

    // 鼠标左键点击寻路
    if (e.type == EVENT_MOUSE_BUTTON_UP && e.button.button == BUTTON_LEFT)
    {
        auto engine = Engine::getInstance();
        int screen_w = engine->getUIWidth();
        int screen_h = engine->getUIHeight();
        int margin = 40;
        int map_display_size = std::min(screen_w - margin * 2, screen_h - margin * 2 - 30);
        float scale = (float)map_display_size / MAINMAP_COORD_COUNT;
        int offset_x = (screen_w - map_display_size) / 2;
        int offset_y = (screen_h - map_display_size) / 2 + 10;

        int mx, my;
        engine->getMouseStateInStartWindow(mx, my);

        // 转换屏幕坐标到地图坐标
        int map_x = (int)((mx - offset_x) / scale);
        int map_y = (int)((my - offset_y) / scale);

        if (map_x >= 0 && map_x < MAINMAP_COORD_COUNT && map_y >= 0 && map_y < MAINMAP_COORD_COUNT)
        {
            // 尝试寻路到目标位置
            if (canWalk(map_x, map_y))
            {
                FindWay(man_x_, man_y_, map_x, map_y);
                setMouseEventPoint(map_x, map_y);
                show_worldmap_ = false;  // 关闭地图开始行走
            }
            else if (isBuilding(map_x, map_y))
            {
                // 点击建筑：寻找入口
                int buiding_x = build_x_layer_.data(map_x, map_y);
                int buiding_y = build_y_layer_.data(map_x, map_y);
                bool found = false;
                for (int ix = buiding_x + 1; ix > buiding_x - 9 && !found; ix--)
                {
                    for (int iy = buiding_y + 1; iy > buiding_y - 9 && !found; iy--)
                    {
                        if (build_x_layer_.data(ix, iy) == buiding_x && build_y_layer_.data(ix, iy) == buiding_y
                            && checkEntrance(ix, iy, true))
                        {
                            // 找入口附近的可走点
                            std::vector<Point> ps;
                            if (canWalk(ix - 1, iy)) ps.push_back({ix - 1, iy});
                            if (canWalk(ix + 1, iy)) ps.push_back({ix + 1, iy});
                            if (canWalk(ix, iy - 1)) ps.push_back({ix, iy - 1});
                            if (canWalk(ix, iy + 1)) ps.push_back({ix, iy + 1});
                            if (!ps.empty())
                            {
                                Point target = ps[0];
                                FindWay(man_x_, man_y_, target.x, target.y);
                                setMouseEventPoint(ix, iy);
                                show_worldmap_ = false;
                                found = true;
                            }
                        }
                    }
                }
            }
        }
    }
}
