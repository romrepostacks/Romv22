// Party Royale music (Phase 5): an original chiptune soundtrack in the GBA handheld style, synthesized live
// with Web Audio (two pulse channels, a triangle bass, noise drums). No audio files; all melodies are new.
// Each track is 8th-note steps: a note name ("C5", "F#4", "Bb3"), "~" to hold the previous note, "." for silence.
// Drums: k kick, s snare, h hi-hat.
const MUSIC_TRACKS = {
  town:{bpm:100,
    lead:'E5 ~ G5 ~ C6 ~ B5 A5 G5 ~ E5 ~ F5 ~ D5 ~ E5 ~ G5 ~ A5 ~ G5 F5 E5 ~ D5 ~ C5 ~ ~ ~',
    harm:'C5 ~ ~ ~ E5 ~ ~ ~ D5 ~ ~ ~ B4 ~ ~ ~ C5 ~ ~ ~ F5 ~ ~ ~ E5 ~ ~ ~ G4 ~ ~ ~',
    bass:'C3 . G3 . C3 . G3 . G2 . D3 . G2 . D3 . C3 . G3 . F2 . C3 . G2 . D3 . C3 . G2 .',
    drum:'k . h . s . h . k . h . s . h . k . h . s . h . k . h . s . h h'},
  route:{bpm:132,
    lead:'D5 ~ G5 ~ B5 ~ A5 G5 A5 ~ B5 ~ D6 ~ B5 ~ C6 ~ B5 A5 G5 ~ E5 ~ F#5 ~ A5 ~ G5 ~ ~ ~',
    harm:'B4 ~ ~ ~ D5 ~ ~ ~ F#5 ~ ~ ~ G5 ~ ~ ~ E5 ~ ~ ~ C5 ~ ~ ~ D5 ~ ~ ~ B4 ~ ~ ~',
    bass:'G2 . D3 . G2 . D3 . D3 . A2 . D3 . A2 . C3 . G2 . C3 . E3 . D3 . A2 . G2 . D3 .',
    drum:'k . h h s . h . k k h . s . h h k . h h s . h . k . h . s s s s'},
  sea:{bpm:96,
    lead:'A4 ~ ~ D5 ~ ~ F#5 ~ E5 ~ ~ D5 ~ ~ B4 ~ A4 ~ ~ D5 ~ ~ F#5 ~ A5 ~ ~ G5 ~ F#5 E5 ~',
    harm:'F#4 ~ ~ ~ ~ ~ ~ ~ G4 ~ ~ ~ ~ ~ ~ ~ F#4 ~ ~ ~ ~ ~ ~ ~ E4 ~ ~ ~ ~ ~ ~ ~',
    bass:'D3 . A3 . D3 . A3 . G2 . D3 . G2 . D3 . D3 . A3 . D3 . A3 . A2 . E3 . A2 . E3 .',
    drum:'k . . h . . s . k . . h . . s . k . . h . . s . k . . h s . s .'},
  cave:{bpm:84,
    lead:'A4 ~ ~ ~ C5 ~ B4 ~ ~ ~ ~ ~ E4 ~ ~ ~ A4 ~ ~ ~ D5 ~ C5 ~ B4 ~ G#4 ~ ~ ~ ~ ~',
    harm:'E4 ~ ~ ~ ~ ~ ~ ~ . . . . . . . . F4 ~ ~ ~ ~ ~ ~ ~ E4 ~ ~ ~ ~ ~ ~ ~',
    bass:'A2 ~ ~ ~ A2 ~ ~ ~ A2 ~ ~ ~ G2 ~ ~ ~ F2 ~ ~ ~ F2 ~ ~ ~ E2 ~ ~ ~ E2 ~ ~ ~',
    drum:'k . . . . . h . k . . . . . h . k . . . . . h . k . . . s . . .'},
  deep:{bpm:72,
    lead:'B4 ~ ~ ~ ~ ~ G4 ~ A4 ~ ~ ~ F#4 ~ ~ ~ E4 ~ ~ ~ ~ ~ G4 ~ B4 ~ ~ ~ D5 ~ ~ ~',
    harm:'E4 ~ ~ ~ ~ ~ ~ ~ D4 ~ ~ ~ ~ ~ ~ ~ C4 ~ ~ ~ ~ ~ ~ ~ D4 ~ ~ ~ ~ ~ ~ ~',
    bass:'E2 ~ ~ ~ B2 ~ ~ ~ D2 ~ ~ ~ A2 ~ ~ ~ C2 ~ ~ ~ G2 ~ ~ ~ D2 ~ ~ ~ A2 ~ ~ ~',
    drum:'. . . . h . . . . . . . h . . . . . . . h . . . . . . . h . . .'},
  wild:{bpm:160,
    lead:'E5 E5 G5 E5 A5 ~ G5 ~ F#5 F#5 A5 F#5 B5 ~ A5 ~ G5 G5 B5 G5 C6 ~ B5 A5 G5 ~ F#5 ~ E5 ~ D#5 ~',
    harm:'B4 ~ ~ ~ C5 ~ ~ ~ A4 ~ ~ ~ D#5 ~ ~ ~ E5 ~ ~ ~ E5 ~ ~ ~ D5 ~ ~ ~ B4 ~ ~ ~',
    bass:'E2 E3 E2 E3 E2 E3 E2 E3 D2 D3 D2 D3 B1 B2 B1 B2 C2 C3 C2 C3 A1 A2 A1 A2 B1 B2 B1 B2 B1 B2 B1 B2',
    drum:'k h s h k h s h k h s h k h s h k h s h k h s h k k s h k s s s'},
  trainer:{bpm:168,
    lead:'A5 ~ E5 ~ A5 B5 C6 ~ B5 ~ G5 ~ E5 ~ G5 ~ F5 ~ C5 ~ F5 G5 A5 ~ G#5 ~ E5 ~ B5 ~ ~ ~',
    harm:'C5 ~ ~ ~ E5 ~ ~ ~ D5 ~ ~ ~ B4 ~ ~ ~ A4 ~ ~ ~ C5 ~ ~ ~ B4 ~ ~ ~ G#4 ~ ~ ~',
    bass:'A1 A2 A1 A2 A1 A2 A1 A2 G1 G2 G1 G2 G1 G2 G1 G2 F1 F2 F1 F2 F1 F2 F1 F2 E1 E2 E1 E2 E1 E2 E1 E2',
    drum:'k h s h k h s h k h s h k h s h k h s h k h s h k k s h k s s s'},
  leader:{bpm:150,
    lead:'D5 ~ ~ A4 D5 ~ F5 ~ E5 ~ ~ C5 E5 ~ G5 ~ F5 ~ A5 ~ G5 ~ F5 ~ E5 ~ C#5 ~ E5 ~ A5 ~',
    harm:'A4 ~ ~ ~ F4 ~ ~ ~ G4 ~ ~ ~ C5 ~ ~ ~ D5 ~ ~ ~ Bb4 ~ ~ ~ A4 ~ ~ ~ C#5 ~ ~ ~',
    bass:'D2 D3 D2 D3 D2 D3 D2 D3 C2 C3 C2 C3 C2 C3 C2 C3 Bb1 Bb2 Bb1 Bb2 G1 G2 G1 G2 A1 A2 A1 A2 A1 A2 A1 A2',
    drum:'k . s . k k s . k . s . k k s h k . s . k k s . k s k s s s s s'},
  champion:{bpm:172,
    lead:'B5 ~ G5 ~ E5 ~ B5 C6 D6 ~ C6 B5 A5 ~ F#5 ~ G5 ~ E5 ~ C6 ~ B5 A5 G5 ~ F#5 ~ D#5 ~ B4 ~',
    harm:'G5 ~ ~ ~ E5 ~ ~ ~ F#5 ~ ~ ~ D5 ~ ~ ~ E5 ~ ~ ~ E5 ~ ~ ~ B4 ~ ~ ~ F#4 ~ ~ ~',
    bass:'E2 E3 E2 E3 E2 E3 E2 E3 D2 D3 D2 D3 B1 B2 B1 B2 C2 C3 C2 C3 A1 A2 A1 A2 B1 B2 B1 B2 B1 B2 B1 B2',
    drum:'k . s . k k s . k . s . k k s h k . s . k k s . k s k s s s s s'},
  // Gym Leaders (#23): Rell fire, Sable water, Orin ground, Iska ghost, Juno electric, Bryn grass, Hale ice, Corvin dragon.
  g_fire:{bpm:168,
    lead:'A5 A5 ~ G5 A5 ~ C6 ~ B5 A5 ~ G5 E5 ~ ~ ~ F5 F5 ~ E5 F5 ~ A5 ~ G#5 ~ B5 ~ E6 ~ ~ ~',
    harm:'E5 ~ ~ ~ E5 ~ ~ ~ D5 ~ ~ ~ B4 ~ ~ ~ C5 ~ ~ ~ C5 ~ ~ ~ B4 ~ ~ ~ G#4 ~ ~ ~',
    bass:'A1 A2 A1 A2 A1 A2 A1 A2 G1 G2 G1 G2 G1 G2 G1 G2 F1 F2 F1 F2 F1 F2 F1 F2 E1 E2 E1 E2 E1 E2 E1 E2',
    drum:'k h s h k k s h k h s h k k s h k h s h k k s h k s k s s s s s'},
  g_water:{bpm:144,
    lead:'D5 F5 A5 D6 ~ A5 F5 ~ C5 E5 G5 C6 ~ G5 E5 ~ Bb4 D5 F5 Bb5 ~ F5 D5 ~ A4 C#5 E5 A5 ~ ~ G5 ~',
    harm:'F4 ~ ~ ~ ~ ~ ~ ~ E4 ~ ~ ~ ~ ~ ~ ~ D4 ~ ~ ~ ~ ~ ~ ~ C#4 ~ ~ ~ ~ ~ ~ ~',
    bass:'D2 . D3 . A2 . D3 . C2 . C3 . G2 . C3 . Bb1 . Bb2 . F2 . Bb2 . A1 . A2 . E2 . A2 .',
    drum:'k . h . s . h h k . h . s . h h k . h . s . h h k . h . s s s s'},
  g_ground:{bpm:132,
    lead:'E5 ~ ~ ~ G5 ~ E5 ~ D5 ~ ~ ~ B4 ~ ~ ~ C5 ~ ~ ~ E5 ~ C5 ~ B4 ~ ~ ~ D#5 ~ ~ ~',
    harm:'B4 ~ ~ ~ ~ ~ ~ ~ G4 ~ ~ ~ ~ ~ ~ ~ G4 ~ ~ ~ ~ ~ ~ ~ F#4 ~ ~ ~ ~ ~ ~ ~',
    bass:'E1 E1 E2 E1 E1 E1 E2 E1 D1 D1 D2 D1 D1 D1 D2 D1 C1 C1 C2 C1 C1 C1 C2 C1 B0 B0 B1 B0 B1 B1 B2 B1',
    drum:'k k s . k k s . k k s . k k s . k k s . k k s . k k s s k s s s'},
  g_ghost:{bpm:120,
    lead:'B4 ~ C5 ~ B4 ~ A#4 ~ B4 ~ ~ ~ F5 ~ ~ ~ E5 ~ D#5 ~ D5 ~ C#5 ~ C5 ~ ~ ~ B4 ~ ~ ~',
    harm:'F4 ~ ~ ~ ~ ~ ~ ~ F4 ~ ~ ~ ~ ~ ~ ~ G4 ~ ~ ~ ~ ~ ~ ~ F#4 ~ ~ ~ ~ ~ ~ ~',
    bass:'B1 ~ ~ B1 ~ ~ B1 ~ F2 ~ ~ F2 ~ ~ F2 ~ E2 ~ ~ E2 ~ ~ E2 ~ F#2 ~ ~ F#2 ~ ~ B1 ~',
    drum:'k . . h . . s . k . . h . . s . k . . h . . s . k . . h s . s h'},
  g_electric:{bpm:176,
    lead:'G5 B5 D6 B5 G5 B5 D6 B5 F#5 A5 D6 A5 F#5 A5 D6 A5 E5 G5 C6 G5 E5 G5 C6 G5 D5 F#5 A5 D6 ~ ~ C6 ~',
    harm:'D5 ~ ~ ~ D5 ~ ~ ~ D5 ~ ~ ~ C5 ~ ~ ~ C5 ~ ~ ~ B4 ~ ~ ~ A4 ~ ~ ~ F#4 ~ ~ ~',
    bass:'G1 G2 G1 G2 G1 G2 G1 G2 D1 D2 D1 D2 D1 D2 D1 D2 C2 C3 C2 C3 C2 C3 C2 C3 D2 D3 D2 D3 D2 D3 D2 D3',
    drum:'k h s h k h s h k h s h k h s h k h s h k h s h k h s h s s s s'},
  g_grass:{bpm:138,
    lead:'C5 ~ F5 ~ A5 ~ G5 F5 G5 ~ ~ ~ C5 ~ ~ ~ Bb4 ~ D5 ~ F5 ~ E5 D5 C5 ~ E5 ~ G5 ~ ~ ~',
    harm:'A4 ~ ~ ~ C5 ~ ~ ~ E4 ~ ~ ~ E4 ~ ~ ~ D4 ~ ~ ~ Bb4 ~ ~ ~ G4 ~ ~ ~ Bb4 ~ ~ ~',
    bass:'F2 . C3 . F2 . C3 . C2 . G2 . C2 . G2 . Bb1 . F2 . Bb1 . F2 . C2 . G2 . C2 . E2 .',
    drum:'k . h . s . h . k . h . s . h . k . h . s . h . k . h . s s s s'},
  g_ice:{bpm:128,
    lead:'F#6 ~ ~ D6 ~ ~ B5 ~ C#6 ~ ~ A5 ~ ~ F#5 ~ G5 ~ ~ B5 ~ ~ D6 ~ C#6 ~ ~ ~ A#5 ~ ~ ~',
    harm:'B4 ~ ~ ~ ~ ~ ~ ~ A4 ~ ~ ~ ~ ~ ~ ~ G4 ~ ~ ~ ~ ~ ~ ~ F#4 ~ ~ ~ ~ ~ ~ ~',
    bass:'B1 . F#2 . B1 . F#2 . A1 . E2 . A1 . E2 . G1 . D2 . G1 . D2 . F#1 . C#2 . F#1 . C#2 .',
    drum:'k . . . h . . . k . . . h . . . k . . . h . . . k . . . s . s .'},
  g_dragon:{bpm:160,
    lead:'C5 ~ G5 ~ Eb5 ~ C6 ~ Bb5 ~ Ab5 ~ G5 ~ ~ ~ Ab5 ~ F5 ~ Eb5 ~ D5 ~ Eb5 ~ F5 ~ G5 ~ B5 ~',
    harm:'Eb5 ~ ~ ~ G4 ~ ~ ~ D5 ~ ~ ~ Eb5 ~ ~ ~ C5 ~ ~ ~ Ab4 ~ ~ ~ G4 ~ ~ ~ D5 ~ ~ ~',
    bass:'C2 C3 C2 C3 C2 C3 C2 C3 Eb2 Eb3 Eb2 Eb3 Eb2 Eb3 Eb2 Eb3 Ab1 Ab2 Ab1 Ab2 F1 F2 F1 F2 G1 G2 G1 G2 G1 G2 G1 G2',
    drum:'k . s . k k s . k . s . k k s h k . s . k k s . k s k s s s s s'},
  legend:{bpm:140,
    lead:'C5 ~ ~ ~ Eb5 ~ ~ ~ G5 ~ ~ ~ F5 Eb5 D5 ~ C5 ~ ~ ~ Ab5 ~ G5 ~ F5 ~ Eb5 ~ D5 ~ ~ ~',
    harm:'G4 ~ ~ ~ G4 ~ ~ ~ Eb5 ~ ~ ~ B4 ~ ~ ~ Eb4 ~ ~ ~ C5 ~ ~ ~ Ab4 ~ ~ ~ B4 ~ ~ ~',
    bass:'C2 C2 C3 C2 C2 C2 C3 C2 Eb2 Eb2 Eb3 Eb2 G1 G1 G2 G1 Ab1 Ab1 Ab2 Ab1 F1 F1 F2 F1 G1 G1 G2 G1 G1 G2 G1 G2',
    drum:'k . h . s . h . k k h . s . h . k . h . s . h . k s s s s s s s'},
  victory:{bpm:140,
    lead:'C5 E5 G5 C6 ~ ~ G5 ~ A5 ~ B5 ~ C6 ~ ~ ~',
    harm:'E4 G4 C5 E5 ~ ~ E5 ~ F5 ~ G5 ~ E5 ~ ~ ~',
    bass:'C3 . G2 . C3 . G2 . F2 . G2 . C3 . . .',
    drum:'k . h . s . h . k . h . s s s .'},
  credits:{bpm:104,
    lead:'C5 ~ F5 ~ A5 ~ G5 F5 E5 ~ G5 ~ C6 ~ ~ ~ Bb5 ~ A5 G5 F5 ~ D5 ~ E5 ~ G5 ~ F5 ~ ~ ~',
    harm:'A4 ~ ~ ~ C5 ~ ~ ~ Bb4 ~ ~ ~ E5 ~ ~ ~ D5 ~ ~ ~ Bb4 ~ ~ ~ C5 ~ ~ ~ A4 ~ ~ ~',
    bass:'F2 . C3 . F2 . C3 . C2 . G2 . C2 . G2 . Bb1 . F2 . Bb1 . D2 . C2 . G2 . F2 . C3 .',
    drum:'k . h . s . h . k . h . s . h . k . h . s . h . k . h h s . h .'},
};
const NOTE_IDX = {C:0, D:2, E:4, F:5, G:7, A:9, B:11};
const noteHz = n=>{ const m = n.match(/^([A-G])([#b]?)(\d)$/); if(!m) return 0; const semi = NOTE_IDX[m[1]] + (m[2]==='#' ? 1 : m[2]==='b' ? -1 : 0) + (+m[3]+1)*12; return 440*Math.pow(2, (semi-69)/12); };
for(const t of Object.values(MUSIC_TRACKS)) for(const ch of ['lead','harm','bass','drum']) t[ch] = t[ch].split(/\s+/);
const LEADER_THEME = {Rell:'g_fire', Sable:'g_water', Orin:'g_ground', Iska:'g_ghost', Juno:'g_electric', Bryn:'g_grass', Hale:'g_ice', Corvin:'g_dragon'};
const Music = {cur:null, want:null, step:0, next:0, bus:null, pulse:{}, noise:null};
const MUSIC_VOL = {OFF:0, LOW:0.35, MID:0.7, HIGH:1};
function musicLevel(){ if(typeof adv!=='undefined' && adv && adv.sound===false) return 0; const v = typeof adv!=='undefined' && adv && adv.music; return MUSIC_VOL[v || 'MID'] ?? 0.7; }
function musicCtx(){
  try{ audioCtx ||= new (window.AudioContext || window.webkitAudioContext)(); }catch(e){ return null; }
  if(audioCtx.state==='suspended') audioCtx.resume().catch(()=>{});
  return audioCtx.state==='running' ? audioCtx : null;
}
// Pulse waves with the handheld's duty cycles (12.5% / 25%), a triangle for bass, and white noise for drums.
function pulseWave(ctx, duty){
  if(Music.pulse[duty]) return Music.pulse[duty];
  const n = 32, re = new Float32Array(n), im = new Float32Array(n);
  for(let k=1; k<n; k++){ re[k] = Math.sin(2*Math.PI*k*duty)/(k*Math.PI); im[k] = (1-Math.cos(2*Math.PI*k*duty))/(k*Math.PI); }
  return Music.pulse[duty] = ctx.createPeriodicWave(re, im);
}
function noiseBuf(ctx){ if(Music.noise) return Music.noise; const b = ctx.createBuffer(1, ctx.sampleRate*0.3, ctx.sampleRate), d = b.getChannelData(0); for(let i=0;i<d.length;i++) d[i] = Math.random()*2-1; return Music.noise = b; }
function tone(ctx, type, hz, t, dur, vol){
  const o = ctx.createOscillator(), g = ctx.createGain();
  if(typeof type==='number') o.setPeriodicWave(pulseWave(ctx, type)); else o.type = type;
  o.frequency.value = hz;
  g.gain.setValueAtTime(0.0001, t); g.gain.exponentialRampToValueAtTime(vol, t+0.008);
  g.gain.exponentialRampToValueAtTime(vol*0.55, t+Math.min(dur*0.5, 0.15)); g.gain.setValueAtTime(vol*0.55, t+dur*0.9); g.gain.exponentialRampToValueAtTime(0.0001, t+dur);
  o.connect(g).connect(Music.bus); o.start(t); o.stop(t+dur+0.02);
}
function drum(ctx, k, t){
  if(k==='k'){ const o = ctx.createOscillator(), g = ctx.createGain(); o.frequency.setValueAtTime(140, t); o.frequency.exponentialRampToValueAtTime(40, t+0.12);
    g.gain.setValueAtTime(0.9, t); g.gain.exponentialRampToValueAtTime(0.0001, t+0.14); o.connect(g).connect(Music.bus); o.start(t); o.stop(t+0.15); return; }
  const s = ctx.createBufferSource(), f = ctx.createBiquadFilter(), g = ctx.createGain(); s.buffer = noiseBuf(ctx);
  f.type = k==='h' ? 'highpass' : 'bandpass'; f.frequency.value = k==='h' ? 7000 : 1800;
  const len = k==='h' ? 0.04 : 0.12; g.gain.setValueAtTime(k==='h' ? 0.25 : 0.6, t); g.gain.exponentialRampToValueAtTime(0.0001, t+len);
  s.connect(f).connect(g).connect(Music.bus); s.start(t); s.stop(t+len+0.01);
}
function musicSchedule(){
  const ctx = audioCtx, tr = MUSIC_TRACKS[Music.cur]; if(!ctx || !tr) return;
  const sd = 60/tr.bpm/2, len = tr.lead.length;
  while(Music.next < ctx.currentTime + 0.3){
    const i = Music.step % len, t = Music.next;
    for(const [ch, type, vol] of [['lead', 0.25, 0.16], ['harm', 0.125, 0.08], ['bass', 'triangle', 0.3]]){
      const tok = tr[ch][i % tr[ch].length]; if(!tok || tok==='.' || tok==='~') continue;
      let hold = 1; while(tr[ch][(i+hold) % tr[ch].length]==='~' && hold < 16) hold++;
      tone(ctx, type, noteHz(tok), t, hold*sd*0.95, vol);
    }
    const d = tr.drum[i % tr.drum.length]; if(d && d!=='.') drum(ctx, d, t);
    Music.next += sd; Music.step++;
  }
}
// Which track fits right now (checked a few times a second; changes fade across).
function musicWanted(){
  if(document.hidden) return null;
  if(typeof musicHush!=='undefined' && musicHush) return null;   // at the PC / while the Nurse heals (#22)
  if(document.getElementById('credits')) return 'credits';
  if(typeof adv==='undefined' || !adv) return document.getElementById('owTitle') ? 'credits' : null;
  const shown = id=>{ const e = document.getElementById(id); return e && !e.classList.contains('hidden'); };
  if(shown('storyResult')) return 'victory';
  if(shown('battle') && typeof state!=='undefined' && state){
    if(state.legendary) return 'legend';
    const tl = state.trainerLoc;
    if(!tl) return 'wild';
    if(tl.champion || /#elite/.test(tl.name||'')) return 'champion';
    return tl.type==='route' ? 'trainer' : tl.type==='gym' && LEADER_THEME[tl.leaderName] || 'leader';
  }
  const loc = LOCATIONS[adv.loc];
  if(loc.deep) return 'deep';
  if(adv.surfing) return 'sea';
  if(loc.theme==='cave') return 'cave';
  return loc.type==='town' || loc.type==='gym' || adv.inside!=null ? 'town' : 'route';
}
function musicTick(){
  const want = musicLevel() > 0 ? musicWanted() : null;
  const ctx = want ? musicCtx() : audioCtx;
  if(!ctx) return;
  if(want!==Music.cur){
    if(Music.bus){ const old = Music.bus; old.gain.cancelScheduledValues(ctx.currentTime); old.gain.setValueAtTime(old.gain.value, ctx.currentTime); old.gain.linearRampToValueAtTime(0.0001, ctx.currentTime+0.35); setTimeout(()=>old.disconnect(), 500); Music.bus = null; }
    Music.cur = want;
    if(want){ Music.bus = ctx.createGain(); Music.bus.gain.value = 0.0001; Music.bus.gain.linearRampToValueAtTime(0.12*musicLevel(), ctx.currentTime+0.4); Music.bus.connect(ctx.destination); Music.level = musicLevel(); Music.step = 0; Music.next = ctx.currentTime + 0.05; }
  }
  if(Music.bus){
    const lv = musicLevel();   // OPTION > MUSIC changed: glide to the new level
    if(lv!==Music.level){ Music.level = lv; Music.bus.gain.cancelScheduledValues(ctx.currentTime); Music.bus.gain.setTargetAtTime(Math.max(0.0001, 0.12*lv), ctx.currentTime, 0.1); }
    musicSchedule();
  }
}
if(typeof window!=='undefined' && typeof setInterval==='function') setInterval(musicTick, 100);
