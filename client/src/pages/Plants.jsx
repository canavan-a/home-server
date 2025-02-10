import { faArrowLeft, faX } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const Plants = () => {
  const navigate = useNavigate();
  const [plants, setPlants] = useState([]);
  const [password, setPassword] = useState(null);

  const [selectedPlant, setSelectedPlant] = useState(null);

  const pollPlants = () => {
    axios
      .get(`https://aidan.house/api/hydrometer/bulk?doorCode=${password}`, {
        doorCode: password,
      })
      .then((res) => {
        setPlants(res.data);
      })
      .catch((err) => {
        console.log(err);
      });
  };

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    let interval;
    if (password != null) {
      pollPlants();
      interval = setInterval(() => {
        pollPlants();
      }, 500);
    }

    return () => {
      if (interval) {
        clearInterval(interval);
      }
    };
  }, [password]);
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
            {selectedPlant != null && (
              <PlantCard
                value={plants[selectedPlant]}
                selectedPlant={selectedPlant}
                setSelectedPlant={setSelectedPlant}
              />
            )}
            {plants &&
              plants.map((value) => {
                return (
                  <PlantRow
                    key={value.ID}
                    value={value}
                    selectedPlant={selectedPlant}
                    setSelectedPlant={setSelectedPlant}
                  />
                );
              })}
            <div className={` text-center text-sm font-mono`}>
              {" "}
              water at 100%
            </div>
          </div>
        </div>
      </div>
    </>
  );
};

const PlantRow = (props) => {
  const { value, setSelectedPlant, selectedPlant } = props;

  const [viewState, setViewState] = useState(true);

  const averageValue = value.PushStack.length
    ? parseInt(
        value.PushStack.reduce((acc, curr) => acc + curr, 0) /
          value.PushStack.length
      )
    : 0;

  return (
    <div
      key={value.ID}
      className={`text-center border-2 flex p-2 hover:cursor-pointer ${
        selectedPlant == value.ID && "border-green-600 text-green-600"
      }`}
      onClick={() => {
        if (selectedPlant == value.ID) {
          setSelectedPlant(null);
        } else {
          setSelectedPlant(value.ID);
        }
      }}
    >
      <div className="font-mono ">{value.Name}</div>
      <div className="flex-grow"></div>
      <div
        className={`font-mono ${
          averageValue > value.WaterThreshold && "text-error font-bold"
        }`}
      >
        {parseInt(100 * (averageValue / value.WaterThreshold))}%
      </div>
    </div>
  );
};

const PlantCard = (props) => {
  const { selectedPlant, setSelectedPlant, value } = props;
  const averageValue = value.PushStack.length
    ? parseInt(
        value.PushStack.reduce((acc, curr) => acc + curr, 0) /
          value.PushStack.length
      )
    : 0;
  return (
    <div
      className={`text-center border-2 flex-col p-2 hover:cursor-pointer h-auto relative`}
    >
      <button
        className="btn btn-xs btn-outline bg-base-100 btn-error absolute -top-2 -right-2"
        onClick={() => {
          setSelectedPlant(null);
        }}
      >
        <FontAwesomeIcon icon={faX} />
      </button>

      <div className="absolute -top-3.5 px-1 py-0 left-1 bg-base-100 font-mono">
        {value.Name}
      </div>
      <div className="mt-5"></div>

      <div className="font-mono flex">
        <div>Current Value</div>
        <div className="flex-grow"></div>
        <div>{averageValue}</div>
      </div>
      <div className="flex-grow"></div>
      <div className="font-mono "></div>
    </div>
  );
};
