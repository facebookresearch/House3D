FROM nvidia/opengl:1.0-glvnd-devel-ubuntu18.04
# Note: this Dockerfile needs to be used with nvidia-docker
# Note: needs nvidia/opengl. nvidia/cuda does not support opengl

MAINTAINER Yuxin Wu

# make apt-get noninteractive
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y \
	libglfw3-dev libglm-dev libx11-dev libegl1-mesa-dev \
	libpng-dev libjpeg-dev python3-opencv \
	python3-dev build-essential pkg-config git curl wget automake libtool

RUN curl -fSsL -O https://bootstrap.pypa.io/get-pip.py && \
	python3 get-pip.py && \
	rm get-pip.py
# tqdm is only used by the tests
RUN pip3 install tqdm


# update git
RUN git clone --recursive https://github.com/facebookresearch/House3D /House3D
ENV TEST_HOUSE /path/to/some/house

# build renderer
WORKDIR /House3D/renderer
ENV PYTHON_CONFIG python3-config
RUN make -j


# install House3D
WORKDIR /House3D
RUN pip3 install -e .

# test renderer
# RUN cd /House3D/renderer && ./objview-offline.bin $TEST_HOUSE
# test python API
# RUN cd /House3D/tests && python3 test-rendering.py $TEST_HOUSE
