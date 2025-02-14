import { faArrowLeft, faX } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";
import {
  LineChart,
  Line,
  CartesianGrid,
  XAxis,
  YAxis,
  ResponsiveContainer,
} from "recharts";

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
      }, 2000);
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
                password={password}
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
  const { selectedPlant, setSelectedPlant, value, password } = props;
  const averageValue = value.PushStack.length
    ? parseInt(
        value.PushStack.reduce((acc, curr) => acc + curr, 0) /
          value.PushStack.length
      )
    : 0;

  const [graphData, setGraphData] = useState(null);
  const [hours, setHours] = useState(1);

  const fetchGraphData = () => {
    setGraphData(null);
    const RECORD_COUNT = 100;
    axios
      .get(
        `https://aidan.house/api/hydrometer/graph?doorCode=${password}&plant=${selectedPlant}&count=${RECORD_COUNT}&hours=${hours}`
      )
      .then((response) => {
        setGraphData(response.data);
        console.log(response.data);
      });
  };

  useEffect(() => {
    fetchGraphData(24);
  }, [selectedPlant, hours]);
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

      {graphData && (
        <ResponsiveContainer width="100%" height={200}>
          <LineChart
            data={graphData.map((item) => ({
              ...item,
              Timestamp: new Date(item.Timestamp).getTime(), // Convert timestamp to milliseconds since epoch
            }))}
          >
            <Line
              type="monotone"
              dataKey="Value"
              stroke="#8884d8"
              dot={false}
            />
            <YAxis
              label={{ angle: -90, position: "insideLeft" }}
              domain={[
                Math.min(...graphData.map((d) => d.Value)) - 20,
                Math.max(...graphData.map((d) => d.Value)) + 20,
              ]}
              ticks={[
                Math.min(...graphData.map((d) => d.Value)),
                Math.max(...graphData.map((d) => d.Value)),
              ]}
            />
          </LineChart>
        </ResponsiveContainer>
      )}
      <div className="font-mono flex-grow">
        <TimeFrameButton value={24 * 7 * 3} hours={hours} setHours={setHours}>
          3 week
        </TimeFrameButton>
        <TimeFrameButton value={48 * 7} hours={hours} setHours={setHours}>
          2 weeks
        </TimeFrameButton>
        <TimeFrameButton value={24 * 7} hours={hours} setHours={setHours}>
          1 week
        </TimeFrameButton>
        <TimeFrameButton value={24} hours={hours} setHours={setHours}>
          24 hours
        </TimeFrameButton>
        <TimeFrameButton value={12} hours={hours} setHours={setHours}>
          12 hours
        </TimeFrameButton>
        <TimeFrameButton value={3} hours={hours} setHours={setHours}>
          3 hours
        </TimeFrameButton>
        <TimeFrameButton value={1} hours={hours} setHours={setHours}>
          1 hour
        </TimeFrameButton>
      </div>
    </div>
  );
};

const TimeFrameButton = (props) => {
  const { value, setHours, hours, children } = props;

  const currentButton = hours == value;

  const click = () => {
    setHours(value);
  };

  return (
    <button
      disabled={currentButton}
      onClick={click}
      className={`btn ${currentButton ? "btn-primary" : "btn-glass"} btn-xs`}
    >
      {children}
    </button>
  );
};
