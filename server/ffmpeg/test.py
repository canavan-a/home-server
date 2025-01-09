import sys
import time

for i in range(1, 11):
    print(i)
    try:
        sys.stdout.flush()  # Try flushing, but ignore any errors
    except OSError:
        pass
    time.sleep(1)