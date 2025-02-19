import {
  faArrowLeft,
  faArrowRight,
  faChevronLeft,
  faChevronRight,
  faPlay,
  faRightFromBracket,
} from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import axios from "axios";
import { useEffect, useRef, useState } from "react";
import { useNavigate } from "react-router-dom";

const STUN_SERVERS = [
  "stun:stun.l.google.com:19302",
  "stun:stun1.l.google.com:19302",
  "stun:stun2.l.google.com:19302",
];

export const Camera = () => {
  const navigate = useNavigate();

  const videoRef = useRef(null);

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

  const moveCameraDynamic = (dir) => {
    axios
      .post(
        `https://aidan.house/api/camera/dynamic_move?controlCommand=${
          dir.toUpperCase() + 500 + dir.toUpperCase()
        }`,
        {
          doorCode: password,
        }
      )
      .then(() => {
        console.log("moved camera");
      })
      .catch((error) => {
        console.log(error);
      });
  };

  useEffect(() => {
    let signalingServer;
    let interval;

    const doPing = () => {
      console.log("pinging");
      const ping = JSON.stringify({ type: "ping" });
      signalingServer.send(ping);
    };

    if (password) {
      console.log("PASSWORD", password);
      signalingServer = new WebSocket(
        `wss:/aidan.house/api/camera/relay?doorCode=${password}`
      );

      signalingServer.onclose = () => {
        navigate("/settings");
      };

      const start = async () => {
        const servers = {
          iceServers: [
            {
              urls: [...STUN_SERVERS],
            },
          ],
        };

        const pc = new RTCPeerConnection(servers);
        const dataChannel = pc.createDataChannel("dummyChannel");

        signalingServer.onmessage = (event) => {
          if (event.data == "pong") {
            console.log("pong");
            return;
          }

          const response = JSON.parse(event.data);

          if (response.type == "answer") {
            console.log("answer");
            console.log(response);
            pc.setRemoteDescription(response);
          } else {
            console.log("adding candidate;");
            console.log(response);
            pc.addIceCandidate(response);
          }
        };

        const offerOptions = {
          offerToReceiveVideo: true,
          iceRestart: true,
        };

        const offer = await pc.createOffer(offerOptions);
        console.log("offer:", offer);
        pc.setLocalDescription(offer);

        signalingServer.send(JSON.stringify(offer));

        pc.onicecandidate = (event) => {
          if (event.candidate) {
            // Send ICE candidate to signaling server
            console.log("my candidate");
            const payload = {
              type: "candidate",
              candidate: event.candidate.candidate,
              sdpMid: event.candidate.sdpMid,
              sdpMLineIndex: event.candidate.sdpMLineIndex,
            };
            signalingServer.send(JSON.stringify(payload));
          }
        };
        pc.onicecandidateerror = (event) => {
          console.error("ICE candidate error:", event);
        };

        pc.ontrack = (event) => {
          const stream = event.streams[0]; // Get the first MediaStream
          console.log("Received stream:", stream);

          // Attach the stream to an audio or video element
          if (stream.getVideoTracks().length > 0) {
            videoRef.current.srcObject = stream;
            videoRef.current.play();
          }
        };
      };
      signalingServer.onopen = () => {
        console.log("starting");
        interval = setInterval(() => {
          doPing();
        }, 8000);
        start();
      };
    } else {
      setPassword(localStorage.getItem("pw"));
    }

    return () => {
      if (signalingServer) {
        signalingServer.close();
      }

      if (interval) {
        clearInterval(interval);
      }
    };
  }, [password]);
  const startVideoStream = () => {
    if (videoRef.current) {
      // Start the video playback after user clicks the button
      alert("starting stream");

      videoRef.current
        .play()
        .then(() => {
          alert("stream started");

          console.log("Video started successfully.");
        })
        .catch((error) => {
          console.error("Failed to start video:", error);
          alert(
            "Autoplay is blocked by your browser. Please interact with the page to play the video."
          );
        });
    } else {
      alert("no video ref");
    }
  };

  return (
    <>
      <div className="relative w-full h-screen">
        {/* Back button */}
        <div className="absolute top-4 right-4 flex z-10">
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

        {/* Video background */}
        <div className="absolute inset-0">
          <video
            ref={videoRef}
            autoPlay={true}
            loop={true}
            muted={true}
            playsInline={true}
            className="w-full h-full object-contain"
          />
        </div>

        {/* Bottom buttons */}
        <div className="absolute bottom-4 left-0 w-full flex items-center justify-center">
          <div className="flex">
            <button
              className="btn btn-md mr-4"
              onClick={() => {
                moveCameraDynamic("r");
              }}
            >
              <FontAwesomeIcon icon={faChevronLeft} />
              <FontAwesomeIcon icon={faChevronLeft} />
            </button>
            <button
              className="btn btn-md"
              onClick={() => {
                moveCamera("r");
              }}
            >
              <FontAwesomeIcon icon={faChevronLeft} />
            </button>
            <button
              onClick={() => startVideoStream()}
              className="btn btn-md mx-2"
              disabled={true}
            >
              {/* <FontAwesomeIcon icon={faPlay} /> */}
            </button>
            <button
              className="btn btn-md"
              onClick={() => {
                moveCamera("l");
              }}
            >
              <FontAwesomeIcon icon={faChevronRight} />
            </button>
            <button
              className="btn btn-md ml-4"
              onClick={() => {
                moveCameraDynamic("l");
              }}
            >
              <FontAwesomeIcon icon={faChevronRight} />
              <FontAwesomeIcon icon={faChevronRight} />
            </button>
          </div>
        </div>
      </div>
    </>
  );
};
