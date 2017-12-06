
# Usage Instructions

## SUNCG Dataset
1. Create obj+mtl files	 following the [instructions in SUNCGToolbox](https://github.com/shurans/SUNCGtoolbox#convert-to-objmtl).
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

1. Compile the renderer and confirm it works, see [renderer/README.md](renderer/README.md) for compilation details.

2. Add `/path/to/House3DRepo` to `PYTHONPATH`. Alternatively, do `pip install --user .` to install House3D.

## Usage

See `tests/test-rendering.py` for usage of rendering API.
See `tests/test-env.py` for an example use of the environment API.
Both will give you an agent in the environment with interactive keyboard control.
