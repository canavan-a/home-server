import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faArrowLeft, faX } from "@fortawesome/free-solid-svg-icons";
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

  useEffect(() => {
    if ("connection" in navigator) {
      const connection =
        navigator.connection ||
        navigator.mozConnection ||
        navigator.webkitConnection;
      console.log("Effective Type:", connection.effectiveType); // e.g., 'wifi', '4g', 'ethernet'
      console.log("Downlink (Mbps):", connection.downlink); // e.g., 10
      console.log("RTT (ms):", connection.rtt); // e.g., 50
      console.log("Save Data Mode:", connection.saveData); // e.g., false
    } else {
      console.log("Network Information API is not supported on this browser.");
    }
  }, []);

  const [getIpOpen, setGetIpOpen] = useState(false);

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
            {addresses.map((value) => {
              return (
                <div key={value} className="text-center border-2 flex p-2">
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
            <div className={`mt-4 text-center `}>
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
            <div className="flex mt-2">
              <div className="w-full">
                <button
                  onClick={() => {
                    setGetIpOpen((prev) => !prev);
                  }}
                  className="btn btn-md  w-full"
                >
                  get my ip address
                </button>
              </div>
            </div>
            {getIpOpen && (
              <GetIpPanel password={password} setInputFunction={setNewIp} />
            )}
          </div>
        </div>
      </div>
    </>
  );
};

const GetIpPanel = (props) => {
  const { password, setInputFunction } = props;
  const [one, setOne] = useState([]);
  const [two, setTwo] = useState([]);

  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    setOne([]);
    setTwo([]);
    fetchIpList(setOne);
  }, []);

  const fetchIpList = (setFunction) => {
    setIsLoading(true);
    axios
      .post(`https://aidan.house/api/netscan`, {
        doorCode: password,
      })
      .then((response) => {
        setFunction(response.data);
      })
      .catch((err) => {
        console.log(err);
      })
      .finally(() => {
        setIsLoading(false);
      });
  };

  useEffect(() => {
    if (one && two) {
      console.log(one);
      console.log(two);
      const diff = getDifference(one, two);
      console.log(diff);
      if (diff.length == 1) {
        setInputFunction(diff);
      }
    }
  }, [one, two]);

  const getDifference = (arr1, arr2) => {
    const difference1 = arr1.filter((item) => !arr2.includes(item));

    const difference2 = arr2.filter((item) => !arr1.includes(item));

    return [...difference1, ...difference2];
  };

  return (
    <div className="flex mt-2">
      <div className="w-full"></div>
      <button
        onClick={() => fetchIpList(setTwo)}
        className="btn btn-md btn-primary"
        disabled={isLoading}
      >
        {isLoading ? (
          <>
            <span className="loading loading-spinner loading-md"></span>
            scannning network
          </>
        ) : (
          <>
            {two.length == 0
              ? "disconnect or connect to wifi then click"
              : "done"}
          </>
        )}
      </button>
      <div className="w-full"></div>
    </div>
  );
};
