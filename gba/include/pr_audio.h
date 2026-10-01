// Music and sound effects on the GBA's four sound channels, after js/music.js and the web game's SFX table:
// two pulse channels (lead and harmony), the wave channel as a triangle bass, and noise drums. Sound effects
// borrow the first pulse channel while they play.
#ifndef PR_AUDIO_H
#define PR_AUDIO_H

namespace pr::audio
{

void init();
void tick();                        // once a frame (pr::frame does it)

// musicWanted(): a track by name ("town", "route", "wild", "g_fire", ...), or nullptr for silence. The same
// track keeps playing; a new one starts from its beginning.
void play_music(const char* name);
[[nodiscard]] const char* current_music();
void set_hush(bool hush);           // musicHush: the PC and the nurse's machine

enum class sfx
{
    SELECT,
    OPEN,
    BUMP,
    JUMP,
    BALL,
    THROW,
    SHAKE,
    POP,
    HIT,
    CAUGHT,
    SAVE,
    PC_ON,
    PC_OFF,
    PC_LOGIN,
    SPOT,
    OBTAIN,
    DOOR,
    HEAL
};
void play(sfx effect);

}

#endif
