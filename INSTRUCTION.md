
# Usage Instructions

## SUNCG Dataset
1. Create obj+mtl files following the [instructions in SUNCGToolbox](https://github.com/shurans/SUNCGtoolbox#convert-to-objmtl).
  Note that the whole dataset is very large. Doing this for dozens or hundreds of houses is fine and probably
  enough for most tasks.
2. Organize the dataset into this structure:

```
SUNCG/
  house/
    00065ecbdd7300d35ef4328ffe871505/
      house.json
      house.mtl
      house.obj
    ...
  texture/
    *.jpg
	object/
	  101/
		  101.mtl
			101.obj
		...
```

## Installation:

House3D runs on Linux/MacOS with or without Nvidia GPUs.

### Use Dockerfile:
If you're on a Linux with Nvidia GPUs,
we recommend using the [Dockerfile](Dockerfile) to run House3D,
so you don't have to worry about the build process.
To use the docker file, you need to install `nvidia-docker`, then run:

```bash
# If this command cannot find `libEGL_nvidia`, your driver installation is incomplete.
nvidia-container-cli list -l | grep libEGL_nvidia
# Build the docker
docker build -t house3d:v0 .
# Run this command only if you need to use GUI within docker
xhost local:root
# Start the container
nvidia-docker run -it --name house3d --net=host \
    --env="DISPLAY" --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
		house3d:v0
# the "env" and "volume" options are only useful if you need to use GUI within docker
# Now you should be inside the container and House3D is ready to use. You can:

cd /House3D/renderer && ./objview-offline.bin $TEST_HOUSE   # offline rendering, produce an image file
cd /House3D/tests && python3 test-rendering.py $TEST_HOUSE --interactive  # interactive rendering with GUI
```

### Install manually:

To build:

1. Compile the renderer and confirm it works, see [renderer/README.md](renderer) for compilation details.

2. Install OpenCV with python bindings. Note that only OpenCV 3 was tested, although OpenCV 2 might work as well.

3. To be able to `import House3D`, add `/path/to/House3DRepo` to `PYTHONPATH`. Alternatively, run `pip install --user .`.

## Usage

See `tests/test-rendering.py` for usage of rendering API.
The docstrings in
[suncg/render.hh](https://github.com/facebookresearch/House3D/blob/master/renderer/suncg/render.hh)
have more detailed explanation on the APIs.

See `tests/test-env.py` for an example use of the environment API.
The environment API is specialized only for the specific task we're working on.
For your own task you may want to customize it, or redefine an environment based on the raw rendering API.

Some houses have weird structures that may not be suitable for your task.
In the RoomNav task, we've manually selected a subset of houses that looks "reasonable".
The list can be found [here](https://github.com/facebookresearch/House3D/releases/download/v0.9/all_houses.json)

## Concurrency Solutions:

1. Rendering many houses in parallel:

A `RenderAPI` can only render the one house specified by `api.loadScene`. To render
different houses, create more instances of `RenderAPI`.

2. Multi-threading:

As required by OpenGL, `objrender.RenderAPI` can only be used in the thread that
creates it and each thread can only have one `RenderAPI` instance.
To do multi-threading, use `objrender.RenderAPIThread`, which is compatible
with `RenderAPI`, but safe to use in any thread. The APIs are compatible, but
`RenderAPIThread` may be slightly slower.

Using multiple instances of `RenderAPI` to render from multiple threads does not
seem to improve the overall rendering throughput, probably due to hardware limitation.
However, rendering from multiple processes does improve rendering throughput.

3. Multi-processing:

`objrender.RenderAPI` is not fork-safe. You cannot share a `RenderAPI` among processes.
To render from multiple processes in parallel, create the `RenderAPI` separately
in each process. Examples can be found in
`tests/benchmark-rendering-multiprocess.py` and `tests/benchmark-env-multiprocess.py`.
