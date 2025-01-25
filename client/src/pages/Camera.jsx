import {
  faArrowLeft,
  faChevronLeft,
  faChevronRight,
  faRightFromBracket,
} from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const Camera = () => {
  const navigate = useNavigate();

  const [password, setPassword] = useState(null);

  const moveCamera = (dir) => {
    axios
      .post(`https://aidan.house/api/camera/move?direction=${dir}`, {
        doorCode: password,
      })
      .then(() => {
        console.log("moved camera");
      })
      .catch((error) => {
        console.log(error);
      });
  };

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
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
          <div className="grid gap-4 grid-cols-2">
            <button
              className="btn btn-sm"
              onClick={() => {
                moveCamera("l");
              }}
            >
              <FontAwesomeIcon icon={faChevronLeft} />
              left
            </button>
            <button
              className="btn btn-sm"
              onClick={() => {
                moveCamera("r");
              }}
            >
              right <FontAwesomeIcon icon={faChevronRight} />
            </button>
          </div>
        </div>
      </div>
    </>
  );
};
