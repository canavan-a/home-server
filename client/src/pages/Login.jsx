import { useState } from "react";
import { useNavigate } from "react-router-dom";
import axios from "axios";

export const Login = () => {
  const navigate = useNavigate();
  const [password, setPassword] = useState("");

  const doOpen = () => {
    axios
      .get("https://aidan.house/api/toggle/" + password)
      .then((response) => {
        console.log(response);
      })
      .catch((err) => {
        alert("could not open door");
      });
  };

  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        <input
          value={password}
          onChange={(e) => {
            setPassword(e.target.value.trim().toLowerCase());
          }}
          placeholder="password"
          className="input input-sm input-bordered"
        ></input>
        <button
          onClick={doOpen}
          disabled={password === ""}
          className="btn btn-sm btn-glass ml-2"
        >
          open
        </button>
      </div>
    </>
  );
};
