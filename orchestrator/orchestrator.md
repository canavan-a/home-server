# Poor Mans kubernetes

## methodology

I have a memory leak, a really bad one in the clipper class.

I need a way to automatically restart the system so I don't get locked out of my house when it crashes.

I'm way too cooked to build out kubernetes and cicd for this project even though that would make the most sense.

Some reasons I am avoiding such kubectling include:

1. complex python env for google coral TPU

2. device connections including camera and usb tpu to attach to the cluster

3. I swear there was another reason....

Anyway, I'm not doing that right now.

## process

I have 3 tmux windows that depend on each other, piping data to each other.

If the server goesdown, all three of these processes die.

Need to poll the server, if its down, launch all 3 services.

Timing is unimportant however, the following order usualy does the trick:

1. ffmpeg
2. mobilenet
3. server

frontend doesn't really go down
