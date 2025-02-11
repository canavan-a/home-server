import tflite_runtime.interpreter as tflite
from tflite_runtime.interpreter import load_delegate

# Ensure the Edge TPU delegate is used for YOLO inference
interpreter = tflite.Interpreter(
    model_path="yolo11n_full_integer_quant_edgetpu.tflite",
    experimental_delegates=[load_delegate('libedgetpu.so.1.0')]
)

# Print out the model details to confirm the Edge TPU is being used
if interpreter._delegate is not None:
    print("Edge TPU is being used for inference.")
else:
    print("Edge TPU is NOT being used.")
