import {
  faCab,
  faCamera,
  faCode,
  faDoorOpen,
  faLeaf,
  faRightFromBracket,
  faTerminal,
} from "@fortawesome/free-solid-svg-icons";
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
                navigate("/camconfig");
              }}
            >
              Camera Configuration <FontAwesomeIcon icon={faTerminal} />
            </button>
            <button
              className="btn btn-glass w-full bg-blue-600"
              onClick={() => {
                navigate("/onetimecode");
              }}
            >
              One Time Code <FontAwesomeIcon icon={faDoorOpen} />
            </button>
            <button
              className="btn btn-glass bg-red-400 w-full"
              onClick={() => {
                navigate("/camera");
              }}
            >
              Camera <FontAwesomeIcon icon={faCamera} />
            </button>
            <button
              className="btn btn-glass bg-green-600 w-full"
              onClick={() => {
                navigate("/plants");
              }}
            >
              Plants <FontAwesomeIcon icon={faLeaf} />
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
