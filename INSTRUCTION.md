
# Usage Instructions

## SUNCG Dataset
1. Create obj+mtl files following the [instructions in SUNCGToolbox](https://github.com/shurans/SUNCGtoolbox#convert-to-objmtl).
  Note that the whole dataset is very large. Doing this for dozens or hundreds of houses is fine and probably
  enough for most tasks.
2. Organize the dataset into this structure:

```
SUNCG/
  house
    00065ecbdd7300d35ef4328ffe871505/
      house.json
      house.mtl
      house.obj
    ...
  texture/
    *.jpg
```

## Installation:

1. Compile the renderer and confirm it works, see [renderer/README.md](renderer) for compilation details.

2. To be able to `import House3D`, add `/path/to/House3DRepo` to `PYTHONPATH`. Alternatively, run `pip install --user .`.

## Usage

See `tests/test-rendering.py` for usage of rendering API.
See `tests/test-env.py` for an example use of the environment API.
Both will give you an agent in the environment with interactive keyboard control.

The environment API is specialized only for the specific task we're working on.
For your own task you may want to customize it based on the raw rendering API.

Some houses have weird structures that may not be suitable for your task.
In the RoomNav task, we've manually selected a subset of houses that looks "reasonable".
The list can be found [here](https://github.com/facebookresearch/House3D/releases/download/v0.9/all_houses.json)

## Concurrency Solutions:

1. Rendering many houses in parallel:

A `RenderAPI` can only render the house specified by `api.loadScene`. To render
different houses, create more instances of `RenderAPI`.

2. Multi-threading:

As required by OpenGL, `objrender.RenderAPI` can only be used in the thread that creates it. 
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
