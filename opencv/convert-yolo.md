# Convert Yolo Model to Coral Edge format (must be done on x86 linux machine)

## using docker

1. pull the docker image

   `docker pull ultralytics/ultralytics:latest`

2. run container and mount bash (from powershell)

   `docker run --rm -it -v ${PWD}:/workspace ultralytics/ultralytics:latest bash`

3. comes with yolo11n convert to coral ai binary

   `python -c "from ultralytics import YOLO; YOLO('yolo11n.pt').export(format='edgetpu')"`

4. copy the file to machine (in separate terminal window)

   `docker cp 9eab72a1c94d:/ultralytics/yolo11n_saved_model/yolo11n_full_integer_quant_edgetpu.tflite ./`

5. You can now use the tflite model

   `model = YOLO("your-model.tflite")`
