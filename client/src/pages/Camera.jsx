import {
  faArrowLeft,
  faChevronLeft,
  faChevronRight,
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

  useEffect(() => {
    let signalingServer;
    let interval;
    if (password) {
      console.log("PASSWORD", password);
      signalingServer = new WebSocket(
        `wss:/aidan.house/api/camera/relay?doorCode=${password}`
      );

      signalingServer.onopen = () => {
        console.log("ws open");
        interval = setInterval(() => {
          doPing();
        }, 1000);
      };
      signalingServer.onclose = () => {
        navigate("/settings");
      };

      const doPing = () => {
        const ping = JSON.stringify({ type: "ping" });
        signalingServer.send(ping);
      };

      const servers = {
        iceServers: [
          {
            urls: [...STUN_SERVERS],
          },
        ],
      };
      const pc = new RTCPeerConnection(servers);

      const start = async () => {
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
          }
          const videoTrack = stream.getVideoTracks()[0];
          videoTrack.onmute = () => {
            console.log("Video track muted try unmute");
            videoTrack.enabled = true; // Try unmuting
          };
          videoTrack.onunmute = () => console.log("Video track unmuted");
          videoTrack.onended = () => console.log("Video track ended");
        };
      };
      signalingServer.onopen = () => {
        console.log("starting");
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

  return (
    <>
      <div className="w-full h-screen flex flex-col items-center justify-center">
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
        <div className="w-[calc(100%-7rem)] h-[calc(100%-25rem)] flex items-center justify-center mt-5 mx-5 mb-2 bg-black">
          <video
            ref={videoRef}
            autoPlay
            playsInline
            className="w-full h-full object-contain"
          />
        </div>
        <div className="w-full flex">
          <div className="flex-grow"></div>
          <button
            className="btn btn-md"
            onClick={() => {
              moveCamera("r");
            }}
          >
            left
          </button>
          <div className="w-60"></div>
          <button
            className="btn btn-md "
            onClick={() => {
              moveCamera("l");
            }}
          >
            right
          </button>

          <div className="flex-grow"></div>
        </div>
      </div>
    </>
  );
};
