import { useState } from "react";
import { Route, Routes, BrowserRouter as Router } from "react-router-dom";
import { Login } from "./pages/Login";
import { Settings } from "./pages/Settings";
import { IpMan } from "./pages/IpMan";
import VideoStream from "./pages/Stream";

function App() {
  const [count, setCount] = useState(0);

  return (
    <div>
      <Router>
        <Routes>
          <Route path="/stream" element={<VideoStream />} />
          <Route path="/settings" element={<Settings />} />
          <Route path="/ipman" element={<IpMan />} />
          <Route path="/*" element={<Login />} />
        </Routes>
      </Router>
    </div>
  );
}

export default App;
