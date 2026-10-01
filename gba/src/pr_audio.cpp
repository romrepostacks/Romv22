#include "pr_audio.h"

#include <cstdint>
#include "bn_math.h"

#include "pr_music_data.h"
#include "pr_state.h"

namespace pr::audio
{

namespace
{
    // Sound registers.
    volatile uint16_t& reg16(uint32_t address)
    {
        return *reinterpret_cast<volatile uint16_t*>(address);
    }
    volatile uint16_t& SOUND1CNT_L = reg16(0x04000060);
    volatile uint16_t& SOUND1CNT_H = reg16(0x04000062);
    volatile uint16_t& SOUND1CNT_X = reg16(0x04000064);
    volatile uint16_t& SOUND2CNT_L = reg16(0x04000068);
    volatile uint16_t& SOUND2CNT_H = reg16(0x0400006C);
    volatile uint16_t& SOUND3CNT_L = reg16(0x04000070);
    volatile uint16_t& SOUND3CNT_H = reg16(0x04000072);
    volatile uint16_t& SOUND3CNT_X = reg16(0x04000074);
    volatile uint16_t& SOUND4CNT_L = reg16(0x04000078);
    volatile uint16_t& SOUND4CNT_H = reg16(0x0400007C);
    volatile uint16_t& SOUNDCNT_L = reg16(0x04000080);
    volatile uint16_t& SOUNDCNT_H = reg16(0x04000082);
    volatile uint16_t& SOUNDCNT_X = reg16(0x04000084);
    volatile uint16_t* const WAVE_RAM = reinterpret_cast<volatile uint16_t*>(0x04000090);

    // 2^(n/12) * 1024, for note frequencies without floating point.
    constexpr int semitone_x1024[] = { 1024, 1085, 1149, 1218, 1290, 1367, 1448, 1534, 1625, 1722, 1825, 1933 };

    // A MIDI note's frequency in Hz x 16.
    int note_hz16(int midi)
    {
        // A4 (69) = 440 Hz. C-1 (0) = 8.1758 Hz = 130.8 x 1/16.
        int octave = midi / 12, semi = midi % 12;
        int hz16 = (131 * semitone_x1024[semi]) >> 10;     // C-1 x 16
        return octave >= 0 ? hz16 << octave : hz16;
    }

    // Pulse channels: f = 131072 / (2048 - x).
    int pulse_rate(int hz16)
    {
        if(hz16 <= 0)
        {
            return 0;
        }
        int x = 2048 - (131072 * 16) / hz16;
        return bn::clamp(x, 0, 2047);
    }

    // Wave channel (32 samples): f = 65536 / (2048 - x).
    int wave_rate(int hz16)
    {
        if(hz16 <= 0)
        {
            return 0;
        }
        int x = 2048 - (65536 * 16) / hz16;
        return bn::clamp(x, 0, 2047);
    }

    const music_data::track* current = nullptr;
    int step = 0;
    int step_timer = 0;                 // in 1/256 frames
    int frames_per_step_256 = 0;
    int note_left[4] = {};              // frames until each channel's note is cut
    bool hush = false;
    int sfx_frames = 0;                 // > 0 while a sound effect has channel 1

    int music_volume()
    {
        const options& o = state().opt;
        if(! o.sound)
        {
            return 0;
        }
        constexpr int levels[] = { 0, 5, 10, 15 };     // OFF, LOW, MID, HIGH (0.35, 0.7, 1)
        return game_active() ? levels[int(o.music)] : levels[int(level4::MID)];
    }

    void silence_channel(int ch)
    {
        switch(ch)
        {
        case 0:
            if(! sfx_frames)
            {
                SOUND1CNT_H = 0;
                SOUND1CNT_X = 0x8000;
            }
            break;
        case 1:
            SOUND2CNT_L = 0;
            SOUND2CNT_H = 0x8000;
            break;
        case 2:
            SOUND3CNT_H = 0;
            break;
        default:
            SOUND4CNT_L = 0;
            SOUND4CNT_H = 0x8000;
            break;
        }
    }

    void silence_all()
    {
        for(int ch = 0; ch < 4; ++ch)
        {
            silence_channel(ch);
            note_left[ch] = 0;
        }
    }

    // A pulse note with the handheld's decay (the web game's attack, then a fall to 55%).
    void pulse_note(int ch, int midi, int volume, int duty, int frames)
    {
        if(volume <= 0)
        {
            return;
        }
        int x = pulse_rate(note_hz16(midi));
        uint16_t env = uint16_t((volume & 15) << 12 | (3 << 8) | (duty << 6));     // decrease, a step every 3/64 s
        if(ch == 0)
        {
            if(sfx_frames)
            {
                return;
            }
            SOUND1CNT_L = 0x0008;       // no sweep
            SOUND1CNT_H = env;
            SOUND1CNT_X = uint16_t(0x8000 | x);
        }
        else
        {
            SOUND2CNT_L = env;
            SOUND2CNT_H = uint16_t(0x8000 | x);
        }
        note_left[ch] = frames;
    }

    void bass_note(int midi, int volume, int frames)
    {
        if(volume <= 0)
        {
            return;
        }
        int x = wave_rate(note_hz16(midi));
        // 100% loud enough for MID and HIGH, 50% for LOW.
        SOUND3CNT_L = 0x80;
        SOUND3CNT_H = uint16_t(volume >= 10 ? 0x2000 : 0x4000);
        SOUND3CNT_X = uint16_t(0x8000 | x);
        note_left[2] = frames;
    }

    void drum(int kind, int volume)
    {
        if(volume <= 0 || ! kind)
        {
            return;
        }
        // Kick: a low rumble that dies fast; snare: mid noise; hi-hat: high, very short.
        int vol = kind == 3 ? volume / 2 : volume * 3 / 4;
        int clock = kind == 1 ? 0x70 : kind == 2 ? 0x40 : 0x10;
        int decay = kind == 3 ? 1 : 2;
        SOUND4CNT_L = uint16_t((vol & 15) << 12 | decay << 8);
        SOUND4CNT_H = uint16_t(0x8000 | clock | 1);
        note_left[3] = kind == 3 ? 3 : 8;
    }

    int hold_steps(const uint8_t* line, int n, int i)
    {
        int hold = 1;
        while(hold < 16 && line[(i + hold) % n] == 1)
        {
            ++hold;
        }
        return hold;
    }

    void music_step()
    {
        const music_data::track& t = *current;
        int vol = hush ? 0 : music_volume();
        int frames = frames_per_step_256 >> 8;
        int i = step;
        // lead: 25% duty, loudest; harmony: 12.5%, softer; bass: the triangle.
        int lead = t.lead[i % t.lead_n], harm = t.harm[i % t.harm_n], bass = t.bass[i % t.bass_n];
        if(lead >= 2)
        {
            pulse_note(0, lead - 2, vol, 1, hold_steps(t.lead, t.lead_n, i) * frames * 95 / 100);
        }
        if(harm >= 2)
        {
            pulse_note(1, harm - 2, vol / 2, 0, hold_steps(t.harm, t.harm_n, i) * frames * 95 / 100);
        }
        if(bass >= 2)
        {
            bass_note(bass - 2, vol, hold_steps(t.bass, t.bass_n, i) * frames * 95 / 100);
        }
        drum(t.drum[i % t.drum_n], vol);
        step = (step + 1) % t.length;
    }

    // Sound effects: [Hz, frames, volume] runs (SFX).
    struct tone
    {
        int16_t hz;
        uint8_t frames;
        uint8_t volume;
    };
    constexpr tone fx_select[] = { {1318, 3, 9} };
    constexpr tone fx_open[] = { {880, 2, 9}, {1318, 3, 9} };
    constexpr tone fx_bump[] = { {98, 5, 12} };
    constexpr tone fx_jump[] = { {523, 3, 9}, {784, 4, 9} };
    constexpr tone fx_ball[] = { {1046, 2, 10} };
    constexpr tone fx_throw[] = { {880, 3, 9}, {660, 3, 9}, {440, 5, 9} };
    constexpr tone fx_shake[] = { {196, 4, 13} };
    constexpr tone fx_pop[] = { {1318, 2, 9}, {880, 4, 9} };
    constexpr tone fx_hit[] = { {220, 3, 8}, {110, 5, 12} };
    constexpr tone fx_caught[] = { {1046, 4, 9}, {0, 2, 0}, {1046, 4, 9}, {1568, 12, 9} };
    constexpr tone fx_save[] = { {784, 6, 9}, {988, 6, 9}, {1175, 6, 9}, {1568, 15, 9} };
    constexpr tone fx_pc_on[] = { {660, 3, 9}, {990, 3, 9}, {1320, 4, 9} };
    constexpr tone fx_pc_off[] = { {1320, 3, 9}, {990, 3, 9}, {660, 4, 9} };
    constexpr tone fx_pc_login[] = { {990, 2, 9}, {1320, 4, 9} };
    constexpr tone fx_spot[] = { {1568, 3, 9}, {2093, 6, 9} };
    constexpr tone fx_obtain[] = { {784, 7, 9}, {784, 4, 9}, {784, 4, 9}, {1046, 18, 9}, {0, 3, 0}, {988, 7, 9}, {1175, 21, 8} };
    constexpr tone fx_door[] = { {392, 3, 10}, {294, 5, 10} };
    constexpr tone fx_heal[] = { {523, 10, 9}, {659, 10, 9}, {784, 10, 9}, {1046, 19, 9}, {0, 5, 0}, {880, 10, 9}, {988, 10, 9},
                                 {1046, 10, 9}, {1318, 36, 9} };

    struct fx_def
    {
        const tone* tones;
        int count;
    };
    template<int N>
    constexpr fx_def def(const tone (&t)[N])
    {
        return { t, N };
    }
    constexpr fx_def effects[] = { def(fx_select), def(fx_open), def(fx_bump), def(fx_jump), def(fx_ball), def(fx_throw),
                                   def(fx_shake), def(fx_pop), def(fx_hit), def(fx_caught), def(fx_save), def(fx_pc_on),
                                   def(fx_pc_off), def(fx_pc_login), def(fx_spot), def(fx_obtain), def(fx_door), def(fx_heal) };

    const fx_def* playing = nullptr;
    int fx_index = 0;
    int fx_left = 0;

    void fx_tone(const tone& t)
    {
        if(t.hz <= 0 || ! t.volume)
        {
            SOUND1CNT_H = 0;
            SOUND1CNT_X = 0x8000;
            return;
        }
        int x = pulse_rate(t.hz * 16);
        SOUND1CNT_L = 0x0008;
        SOUND1CNT_H = uint16_t((t.volume & 15) << 12 | (2 << 6));
        SOUND1CNT_X = uint16_t(0x8000 | x);
    }
}

void init()
{
    SOUNDCNT_X = 0x80;                          // master on
    SOUNDCNT_L = 0xFF77;                        // all four channels, both sides, full volume
    SOUNDCNT_H = 0x0002;                        // DMG channels at 100%
    // The bass's triangle: 32 4-bit samples rising and falling (written to bank 0 while bank 1 plays).
    SOUND3CNT_L = 0x40;
    for(int i = 0; i < 8; ++i)
    {
        // Samples 0..15 then 15..0, two per byte, high nibble first.
        int s[4];
        for(int k = 0; k < 4; ++k)
        {
            int n = i * 4 + k;
            s[k] = n < 16 ? n : 31 - n;
        }
        WAVE_RAM[i] = uint16_t((s[0] << 4 | s[1]) | (s[2] << 4 | s[3]) << 8);
    }
    SOUND3CNT_L = 0x00;
    silence_all();
}

void play_music(const char* name)
{
    if(! name)
    {
        current = nullptr;
        silence_all();
        return;
    }
    if(current && bn::string_view(current->name) == bn::string_view(name))
    {
        return;
    }
    for(const music_data::track& t : music_data::tracks)
    {
        if(bn::string_view(t.name) == bn::string_view(name))
        {
            current = &t;
            step = 0;
            step_timer = 0;
            // An 8th note at this tempo: 60 / bpm / 2 seconds.
            frames_per_step_256 = (60 * 60 * 256) / (t.bpm * 2);
            silence_all();
            return;
        }
    }
}

const char* current_music()
{
    return current ? current->name : nullptr;
}

void set_hush(bool on)
{
    hush = on;
    if(on)
    {
        silence_all();
    }
}

void play(sfx effect)
{
    if(! state().opt.sound && game_active())
    {
        return;
    }
    playing = &effects[int(effect)];
    fx_index = 0;
    fx_left = playing->tones[0].frames;
    sfx_frames = 1;
    fx_tone(playing->tones[0]);
}

void tick()
{
    // A sound effect has channel 1 until it ends.
    if(playing)
    {
        if(--fx_left <= 0)
        {
            ++fx_index;
            if(fx_index >= playing->count)
            {
                playing = nullptr;
                sfx_frames = 0;
                SOUND1CNT_H = 0;
                SOUND1CNT_X = 0x8000;
            }
            else
            {
                fx_left = playing->tones[fx_index].frames;
                fx_tone(playing->tones[fx_index]);
            }
        }
    }
    for(int ch = 0; ch < 4; ++ch)
    {
        if(note_left[ch] > 0 && --note_left[ch] == 0)
        {
            silence_channel(ch);
        }
    }
    if(! current)
    {
        return;
    }
    step_timer += 256;
    while(step_timer >= frames_per_step_256)
    {
        step_timer -= frames_per_step_256;
        music_step();
    }
}

}
