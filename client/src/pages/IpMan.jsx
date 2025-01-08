import axios from "axios";
import { useEffect, useState } from "react";

export const IpMan = () => {
  const [password, setPassword] = useState(null);
  const [addresses, setAddresses] = useState([]);

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    if (password != null) {
      getIpList();
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
  return (
    <>
      <div className="w-full h-screen flex items-center justify-center">
        <div className="w-full max-w-md p-4">
          <div className="grid gap-4">
            {addresses.map((value) => {
              return (
                <div className="font-mono text-center border-2">{value}</div>
              );
            })}
          </div>
        </div>
      </div>
    </>
  );
};
