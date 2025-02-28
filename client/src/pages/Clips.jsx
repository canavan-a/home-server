import { faArrowLeft } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";

export const Clips = () => {
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();

  const [password, setPassword] = useState(null);

  const [clipList, setClipList] = useState([]);

  const [selected, setSelected] = useState(null);

  const [clipData, setClipData] = useState(null);

  const [clipLoading, setClipLoading] = useState(false);

  const [clipCache, setClipCache] = useState({});

  useEffect(() => {
    if (selected) {
      if (clipCache[selected]) {
        setClipData(clipCache[selected]);
      } else {
        setClipData(null);
        setClipLoading(true);
        axios
          .get(
            `https://aidan.house/api/clipper/download?id=${selected}&doorCode=${password}`,
            { responseType: "blob" }
          )
          .then((response) => {
            const blobUrl = URL.createObjectURL(response.data);
            console.log(blobUrl);
            setClipCache({ ...clipCache, [selected.toString()]: blobUrl });
            setClipData(blobUrl);
            setClipLoading(false);
          })
          .catch((error) => {
            setClipLoading(false);
            console.log(error);
          });
      }
    }
  }, [selected]);

  const getClipList = () => {
    setLoading(true);
    axios
      .get(`https://aidan.house/api/clipper/list?doorCode=${password}`)
      .then((response) => {
        console.log(response.data);
        setLoading(false);
        setClipList(response.data);
      })
      .catch((error) => {
        setLoading(false);
        console.log(error);
      });
  };

  useEffect(() => {
    setPassword(localStorage.getItem("pw"));
    if (password != null) {
      getClipList();
    }
  }, [password]);

  const formattedDate = (date) =>
    new Intl.DateTimeFormat("en-US", {
      year: "numeric",
      month: "long",
      day: "numeric",
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit",
    }).format(date);
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
            <div className="text-center flex p-2">
              <div className="flex-grow"></div>
              {loading ? (
                <span className="loading loading-infinity loading-lg"></span>
              ) : (
                <div className="text-4xl text-center w-full font-mono text-primary"></div>
              )}
              <div className="flex-grow"></div>
            </div>
            <div className="text-center flex p-2"></div>

            {selected && (
              <>
                {clipLoading && (
                  <div className="w-full flex">
                    <div className="flex-grow"></div>
                    <span className="loading loading-infinity loading-lg"></span>
                    <div className="flex-grow"></div>
                  </div>
                )}
                {clipData && (
                  <video
                    muted={true}
                    controls={true}
                    autoPlay={true}
                    src={clipData}
                  />
                )}
              </>
            )}
            <div
              className={`${
                selected ? "max-h-60" : "max-h-96"
              } overflow-auto overflow-y-scroll flex-col scrollbar-none`}
            >
              {clipList.map((value) => (
                <button
                  key={value.ID}
                  className={`flex mb-2 btn ${
                    selected == value.ID ? "btn-info" : "btn-glass"
                  } w-full`}
                  onClick={() => {
                    if (selected == value.ID) {
                      setSelected(null);
                    } else {
                      setSelected(value.ID);
                    }
                  }}
                >
                  <div className="flex-grow">{value.ID}</div>
                  <div className="flex-shrink">
                    {formattedDate(new Date(value.Timestamp))}
                  </div>
                  <div className="flex-grow"></div>
                </button>
              ))}
            </div>
            <div className="text-center flex p-2 ">
              <div className="flex-grow"></div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};
