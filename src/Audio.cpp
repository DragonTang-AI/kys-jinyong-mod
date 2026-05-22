#include "Audio.h"
#include "Engine.h"
#include "GameUtil.h"
#include "PlatformMobile.h"
#include "filefunc.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <format>

namespace
{

bool path_ends_with_mid_ci(const std::string& path)
{
    constexpr char suf[] = ".mid";
    const size_t n = path.size();
    if (n < sizeof(suf) - 1)
    {
        return false;
    }
    const size_t base = n - (sizeof(suf) - 1);
    for (size_t i = 0; i + 1 < sizeof(suf); ++i)
    {
        char a = path[base + i];
        char b = suf[i];
        if (a >= 'A' && a <= 'Z')
        {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (a != b)
        {
            return false;
        }
    }
    return true;
}

/** 避免因偶然出现 "MThd"/"RIFF" 误判：校验 SMF 头块长度是否与标准一致 */
bool plausible_smf_header(const uint8_t* p, size_t remaining)
{
    if (remaining < 14)
    {
        return false;
    }
    if (std::memcmp(p, "MThd", 4) != 0)
    {
        return false;
    }
    const uint32_t chunklen = (static_cast<uint32_t>(p[4]) << 24) | (static_cast<uint32_t>(p[5]) << 16)
        | (static_cast<uint32_t>(p[6]) << 8) | static_cast<uint32_t>(p[7]);
    return chunklen >= 6 && chunklen <= 0x02000000u;
}

bool plausible_rmid_header(const uint8_t* p, size_t remaining)
{
    return remaining >= 12 && std::memcmp(p, "RIFF", 4) == 0 && std::memcmp(p + 8, "RMID", 4) == 0;
}

/** 在首部若干字节内定位标准 SMF (MThd) 或 RIFF RMID（与 TiMidity read_midi_file 一致） */
size_t find_midi_container_offset(const uint8_t* p, size_t len, size_t max_scan)
{
    const size_t cap = len < max_scan ? len : max_scan;
    for (size_t i = 0; i + 4 <= cap; ++i)
    {
        const size_t rem = len - i;
        if (plausible_smf_header(p + i, rem) || plausible_rmid_header(p + i, rem))
        {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

void log_midi_preview(const char* label, const std::vector<uint8_t>& buf)
{
    if (buf.empty())
    {
        return;
    }
    const size_t n = std::min<size_t>(32, buf.size());
    std::string hex;
    hex.reserve(n * 3);
    for (size_t i = 0; i < n; ++i)
    {
        hex += std::format("{:02X} ", buf[i]);
    }
    LOG("{} first {} bytes: {}\n", label, n, hex);
}

}  // namespace

Audio::Audio()
{
    {
        const std::string timi_cfg = std::format("{}music/timidity.cfg", GameUtil::PATH());
        if (filefunc::fileExist(timi_cfg))
        {
            SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "TIMIDITY_CFG", timi_cfg.c_str(), true);
        }
    }

    MIX_Init();

#ifdef KYS_MOBILE_EXTRACT_ASSETS
    {
        bool have_timidity = false;
        const int nd = MIX_GetNumAudioDecoders();
        for (int i = 0; i < nd; ++i)
        {
            const char* name = MIX_GetAudioDecoder(i);
            if (name != nullptr && name[0] != '\0' && SDL_strcasecmp(name, "TIMIDITY") == 0)
            {
                have_timidity = true;
                break;
            }
        }
        if (!have_timidity)
        {
            LOG(
                "kys-cpp: SDL_mixer 未注册 TIMIDITY（通常是 timidity.cfg 解析失败或未重编 Mixer）。请将 {}music/timidity.cfg 内音色路径改为指向包内目录。SDL: {}\n",
                GameUtil::PATH(),
                SDL_GetError());
        }
        else if (!filefunc::fileExist(GameUtil::PATH() + "music/timidity.cfg"))
        {
            LOG(
                "kys-cpp: 未找到 {}music/timidity.cfg ，MIDI 可能无声或音色缺失。\n",
                GameUtil::PATH());
        }
    }
#endif

    SDL_AudioSpec spec;
    spec.freq = 22500;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    mixer_ = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (mixer_ == nullptr)
    {
        LOG("MIX_CreateMixerDevice error: {}\n", SDL_GetError());
    }
    if (mixer_)
    {
        track_music_ = MIX_CreateTrack(mixer_);
        track_wav_.resize(20);
        for (auto& t : track_wav_)
        {
            t = MIX_CreateTrack(mixer_);
        }
    }
    init();
}

Audio::~Audio()
{
    //如使用SDL_Mixer播放音频，则销毁应该在SDL_Quit之前，此处的单例设计无法保证，故暂时不处理
    /*
    for (auto m : music_)
    {
        MIX_DestroyAudio(m);
    }
    for (auto a : asound_)
    {
        MIX_DestroyAudio(a);
    }
    for (auto e : esound_)
    {
        MIX_DestroyAudio(e);
    }
    MIX_Quit();
    */
}

void Audio::init()
{
    // Lazy loading: pre-allocate vectors with nullptr, load on demand in playXXX()
    // This avoids blocking startup with 300+ file I/O operations
    music_.resize(100, nullptr);
    asound_.resize(100, nullptr);
    esound_.resize(100, nullptr);
}

void Audio::playMusic(int num)
{
    if (num < 0 || num >= (int)music_.size()) return;
    
    // Lazy load on first play
    if (music_[num] == nullptr)
    {
        std::string music_path;
        const std::string music_dir = GameUtil::PATH() + "music/";
#ifdef KYS_MOBILE_EXTRACT_ASSETS
        for (const char* ext : { ".mp3", ".ogg", ".wav", ".mid" })
        {
            auto p = std::format("{}{}{}", music_dir, num, ext);
            if (filefunc::fileExist(p))
            {
                music_path = std::move(p);
                break;
            }
        }
#else
        music_path = std::format("{}music/{}.mid", GameUtil::PATH(), num);
        if (!filefunc::fileExist(music_path))
        {
            music_path = std::format("{}music/{}.mp3", GameUtil::PATH(), num);
        }
#endif
        if (!music_path.empty() && filefunc::fileExist(music_path))
        {
            music_[num] = loadMusic(music_path);
        }
    }
    
    if (music_[num] == nullptr) return;
    
    stopMusic();
    playMusic(music_[num]);
    current_music_ = music_[num];
    current_music_index_ = num;
}

void Audio::playASound(int num, int volume)
{
    if (num < 0 || num >= (int)asound_.size()) return;
    
    // Lazy load on first play
    if (asound_[num] == nullptr)
    {
        std::string asound_path = std::format("{}sound/atk{:02}.wav", GameUtil::PATH(), num);
        if (filefunc::fileExist(asound_path))
        {
            asound_[num] = loadWav(asound_path);
        }
    }
    
    if (asound_[num] == nullptr) return;
    
    if (volume < 0 || volume > 100) volume = volume_wav_;
    playWav(asound_[num], volume);
    current_sound_ = asound_[num];
}

void Audio::playESound(int num, int volume)
{
    if (num < 0 || num >= (int)esound_.size()) return;
    
    // Lazy load on first play
    if (esound_[num] == nullptr)
    {
        std::string esound_path = std::format("{}sound/e{:02}.wav", GameUtil::PATH(), num);
        if (filefunc::fileExist(esound_path))
        {
            esound_[num] = loadWav(esound_path);
        }
    }
    
    if (esound_[num] == nullptr) return;
    
    if (volume < 0 || volume > 100) volume = volume_wav_;
    playWav(esound_[num], volume);
    current_sound_ = esound_[num];
}

void Audio::pauseMusic()
{
    if (track_music_)
    {
        MIX_PauseTrack(track_music_);
    }
}

void Audio::resumeMusic()
{
    if (track_music_)
    {
        MIX_ResumeTrack(track_music_);
    }
}

void Audio::stopMusic()
{
    if (track_music_)
    {
        MIX_StopTrack(track_music_, 1000);
    }
}

void Audio::stopWav()
{
    if (!track_wav_.empty() && track_wav_[0])
    {
        MIX_StopTrack(track_wav_[0], 0);
    }
}

void Audio::playVoice(int voice_id, int volume)
{
    stopWav();
    if (voice_.find(voice_id) == voice_.end())
    {
        auto filename = std::format("{}voice/{}.mp3", GameUtil::PATH(), voice_id);
        if (!filefunc::fileExist(filename))
        {
            filename = std::format("{}voice/{}.wav", GameUtil::PATH(), voice_id);
        }
        voice_[voice_id] = loadWav(filename);
    }
    playWav(voice_[voice_id], volume, 0);    //使用一个固定的track来播放语音，每次只能有一个语音在播放
    current_sound_ = voice_[voice_id];
}

MUSIC Audio::loadMusic(const std::string& file)
{
#ifndef KYS_MOBILE_EXTRACT_ASSETS
    if (file.contains(".mid"))
    {
        Prop pro;
        auto io = SDL_IOFromFile(file.c_str(), "rb");
        pro.set(MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        pro.set(MIX_PROP_AUDIO_DECODER_STRING, "fluidsynth");
        pro.set("SDL_mixer.decoder.fluidsynth.soundfont_path", (GameUtil::PATH() + "music/mid.sf2").c_str());
        auto m = MIX_LoadAudioWithProperties(pro.id());
        SDL_CloseIO(io);
        return m;
    }
#else
    /* 移动端：Timidity；若包内带自定义头，先裁到 MThd/RIFF 再喂给解码器（否则 init_audio 直接失败） */
    if (path_ends_with_mid_ci(file))
    {
        size_t raw_sz = 0;
        void* raw = SDL_LoadFile(file.c_str(), &raw_sz);
        if (!raw || raw_sz < 14)
        {
            SDL_free(raw);
            LOG("midi read failed or too small: {}\n", file);
            return nullptr;
        }
        const auto* bytes = static_cast<const uint8_t*>(raw);
        const size_t off = find_midi_container_offset(bytes, raw_sz, 262144u);
        if (off == static_cast<size_t>(-1))
        {
            log_midi_preview("midi raw preview (no SMF)", std::vector<uint8_t>(bytes, bytes + std::min<size_t>(raw_sz, 64)));
            SDL_free(raw);
            LOG("midi has no plausible MThd/RMID header in first 256KiB: {}\n", file);
            return nullptr;
        }
        std::vector<uint8_t> buf(bytes + off, bytes + raw_sz);
        SDL_free(raw);

        log_midi_preview("midi trimmed preview", buf);

        SDL_IOStream* io = SDL_IOFromConstMem(buf.data(), buf.size());
        if (!io)
        {
            LOG("SDL_IOFromConstMem failed: {}\n", file);
            return nullptr;
        }
        Prop pro;
        pro.set(MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        pro.set(MIX_PROP_AUDIO_DECODER_STRING, "TIMIDITY");
        pro.set(MIX_PROP_AUDIO_LOAD_SKIP_METADATA_TAGS_BOOLEAN, true);
        if (mixer_)
        {
            pro.set(MIX_PROP_AUDIO_LOAD_PREFERRED_MIXER_POINTER, mixer_);
        }
        MUSIC m = MIX_LoadAudioWithProperties(pro.id());
        SDL_CloseIO(io);
        if (m == nullptr)
        {
            LOG("TIMIDITY load failed {}: {}\n", file, SDL_GetError());
            return nullptr;
        }
        midi_backing_.push_back(std::move(buf));
        return m;
    }
#endif
    if (!mixer_)
    {
        return nullptr;
    }
    return MIX_LoadAudio(mixer_, file.c_str(), false);
}

WAV Audio::loadWav(const std::string& file)
{
    if (!mixer_)
    {
        return nullptr;
    }
    return MIX_LoadAudio(mixer_, file.c_str(), false);
}

void Audio::playMusic(MUSIC m)
{
    if (!mixer_ || !track_music_ || m == nullptr)
    {
        return;
    }
    MIX_SetTrackGain(track_music_, volume_ / 100.0);
    MIX_SetTrackAudio(track_music_, m);
    Prop pro;
    pro.set(MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, 50);
    pro.set(MIX_PROP_PLAY_LOOPS_NUMBER, -1);
    MIX_PlayTrack(track_music_, pro.id());
}

void Audio::playWav(WAV w, int volume, int track_num)
{
    if (!mixer_ || w == nullptr)
    {
        return;
    }
    if (volume < 0 || volume > 100)
    {
        volume = volume_wav_;
    }
    int tnum = track_num;
    if (tnum < 0)
    {
        tnum = current_track_num_;
    }
    if (tnum < 0 || tnum >= static_cast<int>(track_wav_.size()) || track_wav_[tnum] == nullptr)
    {
        return;
    }
    MIX_SetTrackGain(track_wav_[tnum], volume_wav_ / 100.0);
    MIX_SetTrackAudio(track_wav_[tnum], w);
    MIX_PlayTrack(track_wav_[tnum], 0);
    current_track_num_++;
    if (current_track_num_ >= track_wav_.size())
    {
        current_track_num_ = 0;
    }
}
