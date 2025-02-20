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

  const [currentSpeed, setCurrentSpeed] = useState(999);

  const [loading, setLoading] = useState(false);

  const getSpeed = () => {
    setLoading(true);
    axios
      .get(`https://aidan.house/api/tracker/speed/get?doorCode=${password}`)
      .then((response) => {
        console.log(response.data);
        setLoading(false);
        setCurrentSpeed(response.data);
      })
      .catch((error) => {
        setLoading(false);
        console.log(error);
      });
  };

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    if (password != null) {
      getSpeed();
    }
  }, [password]);

  const updateSpeed = () => {
    setNewSpeed("");
    axios
      .get(
        `https://aidan.house/api/tracker/speed/set?speed=${newSpeed}&doorCode=${password}`
      )
      .then((response) => {
        getSpeed();
      })
      .catch((error) => {});
  };

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
              <div className="flex-grow"></div>
              {loading ? (
                <span className="loading loading-infinity loading-lg"></span>
              ) : (
                <div className="text-4xl text-center w-full font-mono text-primary">
                  {currentSpeed}
                </div>
              )}
              <div className="flex-grow"></div>
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
