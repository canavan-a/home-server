import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import axios from "axios";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faBars } from "@fortawesome/free-solid-svg-icons";

export const Login = () => {
  const LOCK_COLOR = "currentColor";

  const navigate = useNavigate();
  const [password, setPassword] = useState("");
  const [loggedIn, setLoggedIn] = useState(false);

  const [doorStatus, setDoorStatus] = useState("");

  useEffect(() => {
    const pw = localStorage.getItem("pw");
    if (pw != null && pw != "") {
      setPassword(pw ?? "");
      setLoggedIn(true);
    }
  }, []);

  const doOpen = () => {
    setDoorStatus("");
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
        localStorage.setItem("pw", password);
        setLoggedIn(true);
      })
      .catch((err) => {
        doOtp();
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
        // alert("could not get status");
      });
  };

  const logOut = () => {
    setPassword("");
    setLoggedIn(false);
    localStorage.setItem("pw", "");
  };

  useEffect(() => {
    let poller;
    if (password != "") {
      getStatus();
      poller = setInterval(() => {
        if (password != "" && loggedIn) {
          getStatus();
        }
      }, 1000);
    }

    return () => {
      if (poller) {
        clearInterval(poller);
      }
    };
  }, [getStatus, password]);

  const openSettings = () => {
    navigate("/settings");
  };

  const doOtp = (code) => {
    axios
      .post(`https://aidan.house/api/onetime/use?code=${password}`)
      .then((response) => {
        alert("door opened");
        setPassword("");
      })
      .catch((err) => {
        // alert("could not get status");
        alert("invalid code");
      });
  };

  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        {loggedIn && (
          <div className="absolute top-4 right-4 flex ">
            {/* <button onClick={logOut} className="btn btn-sm btn-glass mr-2">
              Log out
            </button> */}
            <button onClick={openSettings} className="btn btn-lg btn-glass">
              <FontAwesomeIcon icon={faBars} />
            </button>
          </div>
        )}
        <div className="w-full max-w-md p-4">
          <div className="grid gap-4">
            {loggedIn ? (
              <>
                <div
                  disabled={password === ""}
                  onClick={doOpen}
                  className=" text-center w-full flex flex-col items-center justify-center space-y-4"
                >
                  {doorStatus == "O" ? (
                    <>
                      <svg
                        width="200"
                        height="200px"
                        viewBox="0 0 16 16"
                        xmlns="http://www.w3.org/2000/svg"
                      >
                        <path
                          fillRule="evenodd"
                          clipRule="evenodd"
                          d="M4 6V4C4 1.79086 5.79086 0 8 0C10.2091 0 12 1.79086 12 4V6H14V16H2V6H4ZM6 4C6 2.89543 6.89543 2 8 2C9.10457 2 10 2.89543 10 4V6H6V4ZM7 13V9H9V13H7Z"
                          fill={LOCK_COLOR}
                        />
                      </svg>
                    </>
                  ) : (
                    <>
                      {doorStatus == "L" ? (
                        <svg
                          width="200px"
                          height="200px"
                          viewBox="0 0 16 16"
                          xmlns="http://www.w3.org/2000/svg"
                        >
                          <path
                            fillRule="evenodd"
                            clipRule="evenodd"
                            d="M11.5 2C10.6716 2 10 2.67157 10 3.5V6H13V16H1V6H8V3.5C8 1.567 9.567 0 11.5 0C13.433 0 15 1.567 15 3.5V4H13V3.5C13 2.67157 12.3284 2 11.5 2ZM9 10H5V12H9V10Z"
                            fill={LOCK_COLOR}
                          />
                        </svg>
                      ) : (
                        <span className="loading loading-infinity loading-lg"></span>
                      )}
                    </>
                  )}
                </div>
              </>
            ) : (
              <>
                <input
                  value={password}
                  onChange={(e) => {
                    setPassword(e.target.value.trim().toLowerCase());
                  }}
                  placeholder="enter code"
                  className="input input-md input-bordered w-full"
                />
                <button
                  onClick={testPassword}
                  disabled={password === ""}
                  className="btn btn-md btn-glass w-full"
                >
                  Enter
                </button>
              </>
            )}
          </div>
        </div>
      </div>
    </>
  );
};
