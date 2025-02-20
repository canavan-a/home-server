import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faArrowLeft, faX } from "@fortawesome/free-solid-svg-icons";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const CamConfig = () => {
  const [password, setPassword] = useState(null);
  const [addresses, setAddresses] = useState([]);

  const navigate = useNavigate();
  const [newSpeed, setNewSpeed] = useState("");

  const [networkActorActive, setNetworkActorActive] = useState(false);

  const [loading, setLoading] = useState(false);

  const getSpeed = () => {
    axios
      .get(`https://aidan.house/api/tracker/toggle?doorCode=${password}`)
      .then(() => {
        console.log("tracker toggled");
        getTrackerStatus();
      })
      .catch((error) => {
        console.log(error);
      });
  };

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    if (password != null) {
    }
  }, [password]);

  const updateSpeed = () => {};

  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        <div className="absolute top-4 right-4 flex ">
          <button
            className="btn btn-glass mr-2"
            onClick={() => {
              navigate("/settings");
            }}
          >
            <FontAwesomeIcon icon={faArrowLeft} />
            back
          </button>
        </div>
        <div className="w-full max-w-md p-4">
          <div className="grid gap-4">
            <div className="text-center flex p-2">
              <div className="text-4xl text-center w-full">hello</div>
            </div>
            <div className="text-center flex p-2">
              <input
                value={newSpeed}
                onChange={(e) => {
                  const value = e.target.value.trim();
                  if (/^$|^-?\d*\.?\d+$/.test(value)) {
                    setNewSpeed(value);
                  }
                }}
                className="input input-sm input-bordered w-full"
              ></input>
              <button
                onClick={updateSpeed}
                disabled={newSpeed == ""}
                className="btn btn-sm btn-success ml-2"
              >
                update
              </button>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};
