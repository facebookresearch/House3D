
# Rendering code of House3D

We provided the [Dockerfile](../Dockerfile) to simplify the build process on Linux + Nvidia GPUs.
Follow the instructions below if you want to build by your own.

Build process may be slightly different on different platforms, due to the dependencies.
These are some platforms we've tested on:

### Ubuntu 16.04
```
apt install libglfw3-dev libglm-dev libx11-dev libegl1-mesa-dev libpng-dev libjpeg-dev
```

### macOS
```bash
brew install glfw jpeg libpng glm pkg-config
# If you've installed findutils without this option, uninstall it first.
brew install findutils --with-default-names
# add /usr/local/bin to PATH
```

### ArchLinux
```bash
pacman -S glfw-x11 libglvnd glm
```

### CentOS 7 with Nvidia GPU
```bash
yum install libX11-devel glfw-devel glm-devel mesa-libGL-devel mesa-libEGL-devel libpng-devel libjpeg-devel autoconf automake libtool
```

Then, install libglvnd:
```bash
cd SOME/DOWNLOAD_DIR/
git clone https://github.com/NVIDIA/libglvnd && cd libglvnd
./autogen.sh && ./configure --prefix=SOME/INSTALL_DIR --disable-egl
# disable-egl is needed, to use libEGL that comes with the driver, but everything else from libglvnd
make && make install
# add INSTALL_DIR/lib/pkgconfig to PKG_CONFIG_PATH
# add INSTALL_DIR/lib to LD_LIBRARY_PATH
```

### Ubuntu 14.04 with Nvidia GPU
```
apt install libx11-dev libegl1-mesa-dev libpng-dev libjpeg-dev
```

Install glfw3 (the one in apt is too old):
```bash
cd SOME/DOWNLOAD_DIR
wget https://github.com/glfw/glfw/releases/download/3.2.1/glfw-3.2.1.zip
unzip glfw-3.2.1.zip && cd glfw-3.2.1
mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=SOME/INSTALL_DIR -DBUILD_SHARED_LIBS=ON
make && make install
# add INSTALL_DIR/lib/pkgconfig to PKG_CONFIG_PATH
# add INSTALL_DIR/lib to LD_LIBRARY_PATH
```

Install glm (the one in apt is too old):
```bash
cd SOME/DOWNLOAD_DIR
wget https://github.com/g-truc/glm/releases/download/0.9.8.4/glm-0.9.8.4.zip
unzip glm-0.9.8.4 && cd glm
mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=SOME/INSTALL_DIR
make && make install
```
Later, remember to compile with `INCLUDE_DIR=-I/path/to/INSTALL_DIR/include make`.

Install EGL headers (the headers in apt is too old):
```
cd INSTALL_DIR
wget -P EGL https://www.khronos.org/registry/EGL/api/EGL/egl.h
wget -P EGL https://www.khronos.org/registry/EGL/api/EGL/eglext.h
wget -P EGL https://www.khronos.org/registry/EGL/api/EGL/eglplatform.h
```
Later, remember to compile `INCLUDE_DIR=-I/path/to/INSTALL_DIR make`.

If multiple `INCLUDE_DIR` need to be added, use `INCLUDE_DIR=-I/path/one -I/path/two`.

Install libglvnd the same way as above.

## Build
Depends gcc >= 4.9 or clang.

```bash
git submodule init
git submodule update
# make sure your environment variables don't include paths you don't need

# linux/macos, system python (or homebrew python on macos):
make
# or PYTHON_CONFIG=python3-config make
# or PYTHON_CONFIG=python2-config make
# depend on the version of python you want to use

# linux, anaconda:
SYSTEM=conda.linux PYTHON_CONFIG=/path/to/anaconda/bin/python3-config make

# macos, anaconda (upgrade anaconda if you see any compilation issues):
SYSTEM=conda.macos PYTHON_CONFIG=/path/to/anaconda/bin/python3-config make
# If using anaconda, you also need to add /path/to/anaconda/lib to LD_LIBRARY_PATH
```

## Use

C++:
```
./test-rectangle.bin [egl/headless/glfw]		# a small tool to verify that rendering works
./objview.bin xx.obj	# viewer (require a display to show images)
./objview-suncg.bin xx.obj ModelCategoryMapping.csv	 # viewer without person
./objview-offline.bin xx.obj # render without display (to test its availability on server)
```

Python:
```
cd /path/to/House3DRepo/tests
export PYTHONPATH=..
python test-rendering.py /path/to/suncg/house/house.obj

# benchmark the rendering framerate:
```
Check `test-rendering.py` for the API usage.
Example data can be found at [releases](https://github.com/facebookresearch/House3D/releases/tag/example-data).

## Choosing the Rendering Backend:

Certain executables (e.g. `objview.bin`) use on-screen rendering, which requires
a screen/display to show the images.
`objview-offline.bin` and the Python API all use off-screen rendering, and has
the following two options on Linux:

1. When the environment variable "DISPLAY" exists, the variable is used to
   connect to an X11 server, and the __GLX rendering backend__ will be used. Note that:

   + This method does not require a discrete GPU.
     + If the X11 server is connected to a discrete or integrated GPU, the GPU
       will be used. On machines with >1 GPUs, it can only use the one connected to the X11 server.
     + If not (e.g. xvfb, vnc), software rendering (mesa) will be used with a
       very low framerate.
   + Certain types of X session (e.g. a ssh-forwarded X session, a VNC session) may not
     support the necessary render features needed.
     If you are under a SSH session with X forwarding, make sure to 
     `unset DISPLAY` to disable the GLX backend.

2. Otherwise, it will use the __EGL rendering backend__, which requires a decent Nvidia GPU.
   It also has the option to choose which GPU to use, therefore you can run 
   multiple rendering instances on __multiple GPUs__.

On Mac, it will always use the CGL backend.

## Speed:

Use the following script to benchmark:
```
python benchmark-rendering-multiprocess.py /path/to/suncg/house/house.obj --num-proc 5 --num-gpu 1
```
The command prints per-process framerate.
The total framerate should reach __1.5k ~ 2.5k frames per second__ on a decent Nvidia GPU.
It also scales well to multiple GPUs if used with the EGL backend.


## Trouble Shooting

Please tell us the following if you encounter any build issues or the code fails to run:

1. Your environment (hardware, OS, driver version), and how you build.
2. `cd` to `renderer/` directory and run `./debug-build.sh`. Include the results in your issues.
3. If you've successfully built some binaries, please include the output of the
   two commands: `./test-rectangle.bin egl`, `./test-rectangle.bin headless`.


### Common Issues:
1. `Assertion "glGetString(GL_VERSION)" FAILED`: try building with libglvnd as mentioned above.
2. `undefined symbol: _ZTVNSt7__cxx1119basic_ostringstreamIcSt11char_traitsIcESaIcEEE` C++ ABI incompatibility.
3. "dynamic module does not define init function": compile-time and run-time python version does not match.
4. X server error: don't ssh with X forwarding. Make sure there is no "DISPLAY" environment variable.
5. "Framebuffer is not complete!": `LD_LIBRARY_PATH` incorrectly set, causing
   the binary to load a different OpenGL library at run time.
6. "Framebuffer is not complete" after opening many instances of renderer: there seems to be a hard limit, depending on the hardwares,
	on the number of rendering context you can use.
7. "[EGL] Detected 0 devices" or "Failed to get function pointer of eglQueryDevicesEXT": EGL not functioning. There could be multiple reasons:
   + Not linking against `libEGL.so` provided by nvidia driver.
   + GPU or driver does not support EGL.
   + Running inside container (e.g. docker) with an old driver may also result
     in such error.
8. EGL detected >0 devices but says "Cannot access /dev/nvidiaX":
  If you're inside cgroup/container, initialize the renderer with a device id from `detect_nvidia_devices()` in `common.py`
