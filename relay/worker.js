// Party Royale feedback relay (Cloudflare Worker). The game POSTs {kind, text, context, website};
// this files a GitHub issue with a token only the Worker holds, so testers need no account.
// Settings (Worker > Settings > Variables and Secrets):
//   GITHUB_TOKEN  secret: a fine-grained token with Issues read & write on the one repo only
//   REPO          e.g. romrepostacks/Romv22
//   ALLOW_ORIGIN  e.g. https://romrepostacks.github.io (only the game's site may post)
const KINDS = ['Bug', 'Looks wrong', 'Feels off', 'Idea', 'Praise'];
const WAIT_MS = 30000;
// ponytail: per-isolate memory, so the rate limit is best effort; use a KV or Durable Object if spam gets through.
const lastSent = new Map();

// Anonymous text must not @-mention (and so notify) GitHub users.
const defuse = s => s.replace(/@/g, '@​');

export default {
  async fetch(req, env){
    const cors = {'Access-Control-Allow-Origin': env.ALLOW_ORIGIN || '*', 'Access-Control-Allow-Methods': 'POST, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type', 'Vary': 'Origin'};
    const reply = (msg, status = 200) => new Response(msg, {status, headers: cors});
    if(req.method === 'OPTIONS') return reply(null, 204);
    if(req.method !== 'POST') return reply('POST only', 405);
    if(env.ALLOW_ORIGIN && req.headers.get('Origin') !== env.ALLOW_ORIGIN) return reply('Forbidden', 403);

    let d;
    try { d = await req.json(); } catch { return reply('Bad request', 400); }
    if(d.website) return reply('ok');   // bot filled the hidden trap field: pretend it worked
    const text = String(d.text || '').trim().slice(0, 2000);
    const context = String(d.context || '').slice(0, 1500);
    const kind = KINDS.includes(d.kind) ? d.kind : 'Bug';
    if(!text) return reply('Empty', 400);

    const ip = req.headers.get('CF-Connecting-IP') || '?', now = Date.now();
    if(now - (lastSent.get(ip) || 0) < WAIT_MS) return reply('Too many', 429);
    lastSent.set(ip, now);

    const r = await fetch(`https://api.github.com/repos/${env.REPO}/issues`, {
      method: 'POST',
      headers: {Authorization: `Bearer ${env.GITHUB_TOKEN}`, Accept: 'application/vnd.github+json',
        'User-Agent': 'party-royale-feedback', 'Content-Type': 'application/json'},
      body: JSON.stringify({title: defuse(`[${kind}] ${text.split('\n')[0].slice(0, 60)}`),
        body: `${defuse(text)}${defuse(context)}\n\n_Sent anonymously from the game._`})});
    if(!r.ok){ lastSent.delete(ip); return reply('GitHub refused: ' + r.status, 502); }
    return reply('ok');
  }
};
