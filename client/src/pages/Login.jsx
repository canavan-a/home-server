import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import axios from "axios";

export const Login = () => {
  const navigate = useNavigate();
  const [password, setPassword] = useState("");
  const [loggedOn, setLoggedIn] = useState(false);

  const [doorStatus, setDoorStatus] = useState("");

  useEffect(() => {
    const pw = localStorage.getItem("pw");
    if (pw != null && pw != "") {
      setPassword(pw ?? "");
      setLoggedIn(true);
    }
  }, []);

  const doOpen = () => {
    axios
      .post("https://aidan.house/api/toggle", { doorCode: password })
      .then((response) => {
        console.log(response);
      })
      .catch((err) => {
        alert("could not open door");
      });
  };

  const testPassword = () => {
    axios
      .post("https://aidan.house/api/test_auth_key", { doorCode: password })
      .then((response) => {
        console.log(response);
        localStorage.setItem(password);
        setLoggedIn(true);
      })
      .catch((err) => {
        alert("could not authenticate");
      });
  };

  const getStatus = () => {
    axios
      .post("https://aidan.house/api/status", { doorCode: password })
      .then((response) => {
        console.log(response);
        setDoorStatus(response.data.status);
      })
      .catch((err) => {
        alert("could not get status");
      });
  };

  const logOut = () => {
    setPassword("");
    localStorage.setItem(pw, "");
  };

  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        {loggedOn && (
          <button
            onClick={logOut}
            className="absolute top-4 right-4 btn btn-sm btn-glass"
          >
            Log out
          </button>
        )}
        <div className="w-full max-w-md p-4">
          <div className="grid gap-4">
            {loggedOn ? (
              <>
                <button
                  onClick={doOpen}
                  disabled={password === ""}
                  className="btn btn-md btn-glass w-full"
                >
                  unlock
                </button>
                <button
                  onClick={getStatus}
                  disabled={password === ""}
                  className="btn btn-md btn-glass w-full"
                >
                  status: {doorStatus}
                </button>
              </>
            ) : (
              <>
                <input
                  value={password}
                  onChange={(e) => {
                    setPassword(e.target.value.trim().toLowerCase());
                  }}
                  placeholder="Password"
                  className="input input-md input-bordered w-full"
                />
                <button
                  onClick={testPassword}
                  disabled={password === ""}
                  className="btn btn-md btn-glass w-full"
                >
                  Log In
                </button>
              </>
            )}
          </div>
        </div>
      </div>
    </>
  );
};
