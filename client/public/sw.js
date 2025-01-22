// do something monkey biner
self.addEventListener("install", (event) => {
  self.skipWaiting(); // Force immediate activation
});

self.addEventListener("activate", (event) => {
  event.waitUntil(self.clients.claim()); // Immediately control any open clients (pages)
});

self.addEventListener("fetch", (event) => {
  event.respondWith(fetch(event.request)); // Basic fetch handler
});
