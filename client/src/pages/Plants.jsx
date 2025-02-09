import { faArrowLeft, faX } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const Plants = () => {
  const navigate = useNavigate();
  const [plants, setPlants] = useState([]);
  const [password, setPassword] = useState(null);

  const pollPlants = () => {
    axios
      .get(`https://aidan.house/api/hydrometer/bulk?doorCode=${password}`, {
        doorCode: password,
      })
      .then((res) => {
        console.log(res.data);
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
            {plants &&
              plants.map((value) => {
                return (
                  <div
                    key={value.ID}
                    className="text-center border-2 flex p-2 bg-green"
                  >
                    <div className="font-mono ">{value.Name}</div>
                    <div className="flex-grow"></div>
                    <div className="font-mono ">
                      {value.PushStack.length
                        ? parseInt(
                            value.PushStack.reduce(
                              (acc, curr) => acc + curr,
                              0
                            ) / value.PushStack.length
                          )
                        : 0}
                    </div>
                  </div>
                );
              })}
            <div className={`mt-10 text-center `}>
              <button onClick={pollPlants} className={`btn btn-lg w-full `}>
                hello plants
              </button>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};
