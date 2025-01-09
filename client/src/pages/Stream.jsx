import React, { useEffect, useRef } from "react";

const VideoStream = () => {
  const videoElementRef = useRef(null);
  const decoderRef = useRef(null);
  const audioDecoderRef = useRef(null);
  const websocketRef = useRef(null);
  const accumulatedDataRef = useRef([]);

  useEffect(() => {
    // Initialize WebSocket and WebCodecs decoder
    const initDecoderAndWebSocket = async () => {
      if (!window.VideoDecoder || !window.AudioDecoder) {
        console.error("WebCodecs is not supported in this browser.");
        return;
      }

      // Set up the VideoDecoder for VP8 video codec
      decoderRef.current = new VideoDecoder({
        output: handleDecodedVideoFrame,
        error: handleDecoderError,
      });

      const videoConfig = {
        codec: "vp8", // VP8 codec for video
        width: 480, // Adjust to match your resolution
        height: 270, // Adjust to match your resolution
      };
      decoderRef.current.configure(videoConfig);

      // Set up the AudioDecoder for Opus audio codec
      audioDecoderRef.current = new AudioDecoder({
        output: handleDecodedAudioFrame,
        error: handleDecoderError,
      });

      const audioConfig = {
        codec: "opus", // Opus codec for audio
        sampleRate: 48000, // Typical for Opus
        numberOfChannels: 1, // Mono audio (as specified in FFmpeg command)
      };
      audioDecoderRef.current.configure(audioConfig);

      // Set up the WebSocket connection to the stream
      websocketRef.current = new WebSocket("ws://localhost:5000/api/conn");

      websocketRef.current.onopen = () => {
        console.log("WebSocket connected");
      };

      websocketRef.current.onmessage = (event) => {
        // Accumulate incoming data chunks from the WebSocket
        accumulatedDataRef.current.push(new Uint8Array(event.data));

        // Try to process accumulated data immediately (not waiting for full frame)
        processAccumulatedData();
      };

      websocketRef.current.onerror = (error) => {
        console.error("WebSocket error:", error);
      };

      websocketRef.current.onclose = () => {
        console.log("WebSocket connection closed");
      };
    };

    // Handle the decoded video frame
    const handleDecodedVideoFrame = (frame) => {
      const video = videoElementRef.current;
      if (video) {
        video.srcObject = frame;
      }
      frame.close(); // Don't forget to close the frame when done
    };

    // Handle the decoded audio frame
    const handleDecodedAudioFrame = (frame) => {
      // Audio processing logic here (e.g., play audio or buffer it)
      console.log("Decoded audio frame:", frame);
      frame.close(); // Close the audio frame when done
    };

    // Handle decoder errors
    const handleDecoderError = (error) => {
      console.error("WebCodecs decoder error:", error);
    };

    // Process accumulated data to decode the next frame
    const processAccumulatedData = () => {
      // Combine all accumulated chunks into a single Uint8Array
      const completeData = new Uint8Array(
        accumulatedDataRef.current.reduce(
          (acc, chunk) => acc.concat(Array.from(chunk)),
          []
        )
      );

      // Check if the incoming data has video or audio frames and decode accordingly
      if (completeData.length > 0) {
        try {
          // Assuming the first byte is indicative of the type of frame (video vs. audio)
          // If it's video, decode it with VideoDecoder
          if (completeData[0] === 0x1f) {
            // Example check for video data (adjust as needed)
            decoderRef.current.decode(completeData);
          } else if (completeData[0] === 0x2f) {
            // Example check for audio data (adjust as needed)
            audioDecoderRef.current.decode(completeData);
          }
          // Clear the accumulated data after decoding
          accumulatedDataRef.current = [];
        } catch (error) {
          console.error("Error decoding data:", error);
        }
      }
    };

    // Initialize everything
    initDecoderAndWebSocket();

    return () => {
      // Clean up WebSocket and VideoDecoder when component unmounts
      if (websocketRef.current) {
        websocketRef.current.close();
      }
      if (decoderRef.current) {
        decoderRef.current.close();
      }
      if (audioDecoderRef.current) {
        audioDecoderRef.current.close();
      }
    };
  }, []);

  return <video ref={videoElementRef} controls />;
};

export default VideoStream;
