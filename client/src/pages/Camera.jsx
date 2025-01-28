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
  const handleUserInteraction = () => {
    videoRef.current.play();
  };

  const [canAutoplay, setCanAutoplay] = useState(false);

  useEffect(() => {
    const checkAutoplaySupport = async () => {
      if (videoRef.current) {
        try {
          // Attempt to play the video
          await videoRef.current.play();
          console.log("Autoplay is supported and enabled.");
          setCanAutoplay(true);
          alert("Autoplay is supported and enabled.");
        } catch (error) {
          console.error("Autoplay is not supported or blocked:", error);
          setCanAutoplay(false);
          alert(
            "Autoplay is not supported or blocked. Please interact with the page to play the video."
          );
        }
      }
    };

    checkAutoplaySupport();
  }, []);

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
              className="btn btn-md"
              onClick={() => {
                moveCamera("r");
              }}
            >
              <FontAwesomeIcon icon={faArrowLeft} />
            </button>
            {canAutoplay ? (
              <div className="mx-2"></div>
            ) : (
              <button
                onClick={handleUserInteraction}
                className="btn btn-md mx-2"
              >
                <FontAwesomeIcon icon={faPlay} />
              </button>
            )}
            <button
              className="btn btn-md"
              onClick={() => {
                moveCamera("l");
              }}
            >
              <FontAwesomeIcon icon={faArrowRight} />
            </button>
          </div>
        </div>
      </div>
    </>
  );
};
