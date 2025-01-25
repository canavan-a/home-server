import { faRightFromBracket } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { useNavigate } from "react-router-dom";

export const Settings = () => {
  const navigate = useNavigate();
  const logOut = () => {
    localStorage.setItem("pw", "");
    navigate();
  };
  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        <div className="w-full max-w-md p-4">
          <div className="grid gap-4">
            <button
              className="btn btn-warning w-full"
              onClick={() => {
                logOut();
                navigate("/");
              }}
            >
              Log Out <FontAwesomeIcon icon={faRightFromBracket} />
            </button>
            <button
              className="btn btn-secondary w-full"
              onClick={() => {
                navigate("/ipman");
              }}
            >
              IP Address Manager
            </button>
            <button
              className="btn btn-secondary w-full"
              onClick={() => {
                navigate("/onetimecode");
              }}
            >
              One Time Code
            </button>
            <button
              className="btn btn-secondary w-full"
              onClick={() => {
                navigate("/camera");
              }}
            >
              Camera
            </button>
            <button
              className="btn btn-glass w-full"
              onClick={() => navigate("/")}
            >
              Back
            </button>
          </div>
        </div>
      </div>
    </>
  );
};
