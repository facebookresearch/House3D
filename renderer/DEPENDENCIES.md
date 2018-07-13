# Install Dependencies on Different Platforms

In a nutshell, you need the following libraries:
+ libjpeg, libpng headers
+ opengl & egl headers
+ a recent version of glfw3 and glm headers
+ x11 headers
+ python headers
+ libglvnd (on certain systems)

The way to install these dependencies is different on various platforms.
These are some platforms we've tested on:

### Ubuntu 16.04
```
apt install libglfw3-dev libglm-dev libx11-dev libegl1-mesa-dev libpng-dev libjpeg-dev
```

### macOS
```bash
brew install glfw glm jpeg libpng pkg-config
# If you've installed findutils without this option, uninstall it first.
brew install findutils --with-default-names
# add /usr/local/bin to PATH
```

### ArchLinux
```bash
pacman -S glfw-x11 libglvnd glm libjpeg-turbo libpng mesa
```

### CentOS 7 with Nvidia GPU
```bash
yum install libX11-devel glfw-devel glm-devel mesa-libGL-devel mesa-libEGL-devel libpng-devel libjpeg-devel autoconf automake libtool
```

Install libglvnd:
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

