import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faX } from "@fortawesome/free-solid-svg-icons";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const IpMan = () => {
  const [password, setPassword] = useState(null);
  const [addresses, setAddresses] = useState([]);

  const navigate = useNavigate();
  const [newIp, setNewIp] = useState("");

  const [networkActorActive, setNetworkActorActive] = useState(false);

  const getNetworkActorStatus = () => {
    axios
      .post("https://aidan.house/api/network_actor/status", {
        doorCode: password,
      })
      .then((response) => {
        console.log(response.data.running);
        setNetworkActorActive(response.data.running);
      })
      .catch((err) => {
        console.log(err);
      });
  };

  const toggleNetworkActorStatus = () => {
    const endpoint = networkActorActive ? "stop" : "start";
    axios
      .post(`https://aidan.house/api/network_actor/${endpoint}`, {
        doorCode: password,
      })
      .then((response) => {
        console.log(response.data.addresses);
        getNetworkActorStatus();
      })
      .catch((err) => {
        // alert("could not get actor status");
      });
  };

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    if (password != null) {
      getIpList();
      getNetworkActorStatus();
    }
  }, [password]);

  const getIpList = () => {
    axios
      .post("https://aidan.house/api/address/list", { doorCode: password })
      .then((response) => {
        console.log(response.data.addresses);
        setAddresses(response.data.addresses);
      })
      .catch((err) => {
        alert("could not get addresses");
      });
  };

  const changeIp = (e) => {
    console.log(e.target.value);
    setNewIp(e.target.value.replace(/\s/g, ""));
  };

  const addIp = () => {
    axios
      .post(`https://aidan.house/api/address/add?address=${newIp}`, {
        doorCode: password,
      })
      .then((response) => {
        getIpList();
        setNewIp("");
      })
      .catch((err) => {
        // alert("could not get addresses");
      });
  };

  const deleteIp = (ip) => {
    axios
      .post(`https://aidan.house/api/address/remove?address=${ip}`, {
        doorCode: password,
      })
      .then((response) => {
        getIpList();
      })
      .catch((err) => {
        // alert("could not get addresses");
      });
  };

  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        <div className="absolute top-4 right-4 flex ">
          <button
            className="btn btn-sm btn-glass mr-2"
            onClick={() => {
              navigate("/settings");
            }}
          >
            back
          </button>
        </div>
        <div className="w-full max-w-md p-4">
          <div className="grid gap-4">
            {addresses.map((value) => {
              return (
                <div className="text-center border-2 flex p-2">
                  <button
                    className="btn btn-xs btn-error relative left-0"
                    onClick={() => {
                      deleteIp(value);
                    }}
                  >
                    <FontAwesomeIcon icon={faX} />
                  </button>
                  <div className="flex-grow"></div>
                  <div className="font-mono ">{value}</div>
                </div>
              );
            })}
            <div className="text-center flex p-2">
              <input
                value={newIp}
                className="input input-sm input-bordered w-full"
                onChange={changeIp}
              ></input>
              <button onClick={addIp} className="btn btn-sm btn-success ml-2">
                {" "}
                add
              </button>
            </div>
            <div className={`mt-10 text-center `}>
              <button
                onClick={toggleNetworkActorStatus}
                className={`btn btn-lg w-full ${
                  networkActorActive ? "btn-error" : "btn-success"
                }`}
              >
                {" "}
                {!networkActorActive ? "activate" : "deactivate"}{" "}
              </button>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};
