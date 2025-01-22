import { faArrowLeft, faX } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { useState } from "react";
import { useNavigate } from "react-router-dom";

export const OneTime = () => {
  const navigate = useNavigate();
  const [codes, setCodes] = useState([]);

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
            {codes.map((value) => {
              return (
                <div key={value} className="text-center border-2 flex p-2">
                  <div className="flex-grow"></div>
                  <div className="font-mono ">{value}</div>
                </div>
              );
            })}
            <div className="text-center flex p-2">
              <input className="input input-sm input-bordered w-full"></input>
              <button className="btn btn-sm btn-success ml-2"> add otp</button>
            </div>
            <div className={`mt-10 text-center `}>
              <button className={`btn btn-lg w-full `}>clear otp codes</button>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};
