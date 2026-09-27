// Network first, so testers always get the newest build; the cache is the offline fallback.
// Same-origin files revalidate with the server (GitHub Pages lets browsers reuse them for 10 minutes).
const CACHE = 'party-royale';
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', e => e.waitUntil(clients.claim()));
self.addEventListener('fetch', e => {
  if(e.request.method !== 'GET') return;
  const same = new URL(e.request.url).origin === location.origin;
  e.respondWith(fetch(same ? e.request.url : e.request, same ? {cache: 'no-cache', credentials: 'same-origin'} : undefined).then(r => {
    if(r.ok || r.type === 'opaque'){ const copy = r.clone(); caches.open(CACHE).then(c => c.put(e.request, copy)); }
    return r;
  }).catch(() => caches.match(e.request, {ignoreSearch: true})));
});
