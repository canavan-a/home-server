import { useEffect } from "react";
import { Route, Routes, BrowserRouter as Router } from "react-router-dom";
import { Login } from "./pages/Login";
import { Settings } from "./pages/Settings";
import { IpMan } from "./pages/IpMan";
import { OneTime } from "./pages/OneTime";
import { Camera } from "./pages/Camera";
import { Plants } from "./pages/Plants";

const registerServiceWorker = () => {
  if ("serviceWorker" in navigator) {
    navigator.serviceWorker
      .register(`/sw.js?v=${import.meta.env.BUILD_HASH}`)
      .then((registration) => {
        // Listen for updates to the service worker
        registration.onupdatefound = () => {
          const installingWorker = registration.installing;
          if (installingWorker) {
            console.log("installing");
            installingWorker.onstatechange = () => {
              if (
                installingWorker.state === "installed" &&
                navigator.serviceWorker.controller
              ) {
                // If there's a new service worker and the page has a controller
                const userWantsToUpdate = window.confirm(
                  "A new version of the app is available. Do you want to update?"
                );
                if (userWantsToUpdate) {
                  window.location.reload(); // Reload the page to apply the new service worker
                }
              }
            };
          }
        };
      })
      .catch((error) => {
        console.error("Service Worker registration failed:", error);
      });
  }
};

document.addEventListener("visibilitychange", registerServiceWorker);

function App() {
  return (
    <div>
      <Router>
        <Routes>
          <Route path="/settings" element={<Settings />} />
          <Route path="/camera" element={<Camera />} />
          <Route path="/onetimecode" element={<OneTime />} />
          <Route path="/ipman" element={<IpMan />} />
          <Route path="/plants" element={<Plants />} />
          <Route path="/*" element={<Login />} />
        </Routes>
      </Router>
    </div>
  );
}

export default App;
