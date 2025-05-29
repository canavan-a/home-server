import { faArrowLeft, faCopy } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useState } from "react";
import { useNavigate, useParams } from "react-router-dom";

export const Clips = () => {
  const [loading, setLoading] = useState(false);
  const navigate = useNavigate();

  const params = useParams();

  const [password, setPassword] = useState(null);

  const [clipList, setClipList] = useState([]);

  const [selected, setSelected] = useState(null);

  const [clipData, setClipData] = useState(null);

  const [clipLoading, setClipLoading] = useState(false);

  const [clipCache, setClipCache] = useState({});

  console.log(params.id);

  useEffect(() => {
    console.log(selected);
    if (selected) {
      console.log("fethcing");
      setClipData(
        `https://aidan.house/api/clipper/download?name=${selected}&doorCode=${password}`
      );
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

  const USE_COUNT = 6;

  const [copied, setCopied] = useState(null);

  const fetchTemplink = (id) => {
    console.log(id.Name);
    setCopied(null);
    axios
      .get(
        `https://aidan.house/api/templink/generate?doorCode=${password}&count=${USE_COUNT}`
      )
      .then((response) => {
        navigator.clipboard
          .writeText(
            `https://aidan.house/api/templink/view?name=${id.Name}&cred=${response.data}`
          )
          .then((res) => {
            setCopied(id.Name);
          });
        console.log(response.data);
      });
  };

  useEffect(() => {
    if (params.id) {
      setSelected(params.id);
    }
  }, [clipList]);
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
              } overflow-auto w-full overflow-x-clip overflow-y-scroll flex-col scrollbar-none`}
            >
              {clipList
                .map((v) => ({
                  ...v,
                  CaptureDate: new Date(v.Timestamp),
                }))
                .sort((a, b) => b.CaptureDate - a.CaptureDate)
                .map((value, index) => (
                  <div className="flex">
                    <button
                      className={`btn mr-1 ${
                        copied == value.Name ? "btn-info" : "btn-glass"
                      } `}
                      onClick={() => fetchTemplink(value)}
                    >
                      <FontAwesomeIcon icon={faCopy} />
                    </button>
                    <button
                      key={value.Name}
                      className={`flex flex-grow mb-2 btn ${
                        selected == value.Name ? "btn-info" : "btn-glass"
                      } w-full`}
                      onClick={() => {
                        if (selected == value.Name) {
                          setSelected(null);
                        } else {
                          setSelected(value.Name);
                        }
                      }}
                    >
                      <div className="flex-grow">{clipList.length - index}</div>
                      <div className="flex-shrink">
                        {formattedDate(new Date(value.Timestamp))}
                      </div>
                      <div className="flex-grow"></div>
                    </button>
                  </div>
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
