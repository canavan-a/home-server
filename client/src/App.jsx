import { useEffect } from "react";
import { Route, Routes, BrowserRouter as Router } from "react-router-dom";
import { Login } from "./pages/Login";
import { Settings } from "./pages/Settings";
import { CamConfig } from "./pages/CamConfig";
import { OneTime } from "./pages/OneTime";
import { Camera } from "./pages/Camera";
import { Plants } from "./pages/Plants";
import { Clips } from "./pages/Clips";
import { Garage } from "./pages/Garage";

function App() {
  return (
    <div>
      <Router>
        <Routes>
          <Route path="/settings" element={<Settings />} />
          <Route path="/camera" element={<Camera />} />
          <Route path="/clips/:id?" element={<Clips />} />
          <Route path="/onetimecode" element={<OneTime />} />
          <Route path="/camconfig" element={<CamConfig />} />
          <Route path="/plants" element={<Plants />} />
          <Route path="/garage" element={<Garage />} />
          <Route path="/*" element={<Login />} />
        </Routes>
      </Router>
    </div>
  );
}

export default App;
