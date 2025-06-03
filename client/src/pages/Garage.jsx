import { faArrowLeft } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const Garage = () => {
  const navigate = useNavigate();
  const [isLoading, setIsLoading] = useState(false);
  const [open, setOpen] = useState(true);
  const [password, setPassword] = useState(null);
  const [doorStatus, setDoorStatus] = useState(0);
  const [doorStatusOpen, setDoorStatusOpen] = useState(0);

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    if (password != null) {
      console.log("trigger start action");
    }
  }, [password]);

  const checkGarageStatus = () => {
    axios
      .get(`https://aidan.house/api/garage/status?doorCode=${password}`)
      .then((response) => {
        console.log("RES", response);
        setDoorStatus(response.data);
        setIsLoading(false);
      })
      .catch((err) => {
        // alert("could not get status");
      });
  };

  const checkGarageStatusOpen = () => {
    axios
      .get(`https://aidan.house/api/garage/status_open?doorCode=${password}`)
      .then((response) => {
        console.log("RES", response);
        setDoorStatusOpen(response.data);
        setIsLoading(false);
      })
      .catch((err) => {
        // alert("could not get status");
      });
  };

  const doOpen = () => {
    setIsLoading(true);
    axios
      .post(`https://aidan.house/api/garage/trigger`, {
        doorCode: password,
      })
      .then((response) => {})
      .catch((err) => {
        // alert("could not get status");
      });
  };

  useEffect(() => {
    setIsLoading(true);
    let interv = undefined;
    if (password) {
      checkGarageStatus();
      checkGarageStatusOpen();
      interv = setInterval(() => {
        checkGarageStatus();
        checkGarageStatusOpen();
      }, 500);
    }

    return () => {
      clearInterval(interv);
    };
  }, [password]);

  return (
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
          {!isLoading ? (
            <div
              onClick={doOpen}
              className=" text-center w-full flex flex-col items-center justify-center space-y-4"
            >
              {doorStatus == 0 && doorStatusOpen == 0 ? (
                <MovingIcon size={200} />
              ) : (
                <>
                  <>{doorStatus == 1 && <GarageLockIcon size={200} />}</>
                  <>{doorStatusOpen == 1 && <GarageAlertIcon size={200} />}</>
                </>
                // <GarageLockIcon size={200} />
              )}
            </div>
          ) : (
            <div className="flex justify-center items-center w-full h-full">
              <span className="loading loading-infinity loading-lg"></span>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

export const GarageAlertIcon = ({
  size = 24,
  color = "currentColor",
  ...props
}) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 24 24"
    width={size}
    height={size}
    fill={color}
    {...props}
  >
    <path
      xmlns="http://www.w3.org/2000/svg"
      fill="currentColor"
      d="M17 20h-2v-9H5v9H3V9l7-4l7 4v11M6 12h8v2H6v-2m0 3h8v2H6v-2m13 0v-5h2v5h-2m0 4v-2h2v2h-2Z"
    />
  </svg>
);

export const GarageLockIcon = ({
  size = 24,
  color = "currentColor",
  ...props
}) => (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 24 24"
    width={size}
    height={size}
    fill={color}
    {...props}
  >
    <path
      xmlns="http://www.w3.org/2000/svg"
      fill="currentColor"
      d="M20.8 16v-1.5c0-1.4-1.4-2.5-2.8-2.5s-2.8 1.1-2.8 2.5V16c-.6 0-1.2.6-1.2 1.2v3.5c0 .7.6 1.3 1.2 1.3h5.5c.7 0 1.3-.6 1.3-1.2v-3.5c0-.7-.6-1.3-1.2-1.3m-1.3 0h-3v-1.5c0-.8.7-1.3 1.5-1.3s1.5.5 1.5 1.3V16M5 12h8v2H5v-2m0 3h7.95c-.53.54-.87 1.24-.95 2H5v-2m7 5H5v-2h7v2m2-9H4v9H2V9l7-4l7 4v1.44c-.81.36-1.5.92-2 1.62V11Z"
    />
  </svg>
);

export const MovingIcon = ({ size = 24, color = "currentColor", ...props }) => (
  <svg
    aria-hidden="true"
    xmlns="http://www.w3.org/2000/svg"
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="none"
    stroke={color}
    strokeWidth="2"
    strokeLinecap="round"
    strokeLinejoin="round"
    {...props}
  >
    <path d="M17.651 7.65a7.131 7.131 0 0 0-12.68 3.15M18.001 4v4h-4m-7.652 8.35a7.13 7.13 0 0 0 12.68-3.15M6 20v-4h4" />
  </svg>
);

{
  /* <svg
  className="w-6 h-6 text-gray-800 dark:text-white"
  aria-hidden="true"
  xmlns="http://www.w3.org/2000/svg"
  width="24"
  height="24"
  fill="none"
  viewBox="0 0 24 24"
>
  <path
    stroke="currentColor"
    strokeLinecap="round"
    strokeLinejoin="round"
    strokeWidth="2"
    d="M17.651 7.65a7.131 7.131 0 0 0-12.68 3.15M18.001 4v4h-4m-7.652 8.35a7.13 7.13 0 0 0 12.68-3.15M6 20v-4h4"
  />
</svg>; */
}
