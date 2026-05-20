#include "Application.h"

#include "PlatformMobile.h"
#include "Audio.h"
#include "Engine.h"
#include "Event.h"
#include "Font.h"
#include "GameUtil.h"
#include "INIReader.h"
#include "TextureManager.h"
#include "TitleScene.h"
#include "Types.h"
#include "UIKeyConfig.h"

Application::Application()
{
}

Application::~Application()
{
}

int Application::run()
{
#ifdef KYS_MOBILE_EXTRACT_ASSETS
    Engine::extractAssetsIfNeeded();
#endif

    auto game = GameUtil::getInstance();
    renderer_ = game->getString("game", "renderer", "");
    title_ = game->getString("game", "title", "All Heroes in Kam Yung Stories");

    auto engine = Engine::getInstance();
    engine->setUISize(1280, 720);
    engine->init(nullptr, 0, 0, renderer_);
    engine->setWindowTitle(title_);
    engine->addEventWatch([](void*, EngineEvent* e) -> bool
        {
            if (e->type == EVENT_QUIT)
            {
                //退出游戏
                return false;
            }
            if (e->type == EVENT_DID_ENTER_BACKGROUND)
            {
                //暂停音频
                Audio::getInstance()->pauseMusic();
            }
            if (e->type == EVENT_DID_ENTER_FOREGROUND)
            {
                //恢复音频
                Audio::getInstance()->resumeMusic();
            }
            return true;
        },
        nullptr);

    //引擎初始化之后才能创建纹理
    engine->createAssistTexture("scene", 1280, 720);

    config();

    auto s = std::make_shared<TitleScene>();    //开始界面
    s->run();

    return 0;
}

void Application::config()
{
    auto game = GameUtil::getInstance();
    //RunNode::setRefreshInterval(game->getReal("game", "refresh_interval", 16));
    Audio::getInstance()->setVolume(game->getInt("music", "volume", 50));
    Audio::getInstance()->setVolumeWav(game->getInt("music", "volumewav", 50));
    Event::getInstance()->setUseScript(game->getInt("game", "use_script", 1));
    Font::getInstance()->setStatMessage(game->getInt("game", "stat_font", 0));
    Font::getInstance()->setSimplified(game->getInt("game", "simplified_chinese", 1));
    TextureManager::getInstance()->setLoadFromPath(game->getInt("game", "png_from_path", 0));
    TextureManager::getInstance()->setLoadAll(game->getInt("game", "load_all_png", 0));
    UIKeyConfig::readFromString(game->getString("game", "key", ""));
    Scene::setKeyWalkDealy(game->getInt("game", "walk_speed", 20));
    RunNode::setUseVirtualStick(game->getInt("game", "use_virtual_stick", 0));
    Scene::setTileScale(game->getInt("game", "tile_scale", 1));
#ifdef KYS_MOBILE_EXTRACT_ASSETS
    RunNode::setUseVirtualStick(game->getInt("game", "use_virtual_stick", 1));
#endif
    // 移动端默认铺满（可减少大黑边）；桌面默认保留宽高比 Letterbox。[game] present_keep_ratio=1 强制留黑边
    {
        const int default_keep_ratio =
#ifdef KYS_MOBILE_EXTRACT_ASSETS
            0
#else
            1
#endif
            ;
        Engine::getInstance()->setKeepRatio(game->getInt("game", "present_keep_ratio", default_keep_ratio) != 0);
    }
    RunNode ::setRenderMessage(game->getInt("game", "render_message", 0));
    Save::getInstance()->setZipSave(game->getInt("game", "zip_save", 1));

    Role::setMaxValue();
    Role::setLevelUpList();
    Item::setSpecialItems();
}
