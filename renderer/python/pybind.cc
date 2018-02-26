// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: pybind.cc

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>


#include "suncg/render.hh"
#include "lib/mat.h"
#include "lib/timer.hh"

#include "house.hh"

using namespace std;
using namespace render;
namespace py = pybind11;

namespace {
//TotalTimerGlobalGuard TGGG;
}

using namespace pybind11::literals;
PYBIND11_MODULE(objrender, m) {
  py::class_<SUNCGRenderAPI>(m, "RenderAPI")
    // device defaults to 0
    .def(py::init<int, int, int>(), "Initialize", "w"_a, "h"_a, "device"_a=0)
    .def("printContextInfo", &SUNCGRenderAPI::printContextInfo)
    .def("getCamera", &SUNCGRenderAPI::getCamera, py::return_value_policy::reference)
    .def("setMode", &SUNCGRenderAPI::setMode)
    .def("loadSceneSUNCG", &SUNCGRenderAPI::loadScene)
    .def("loadScene", &SUNCGRenderAPI::loadScene)
    .def("resolution", &SUNCGRenderAPI::resolution)
    .def("render", &SUNCGRenderAPI::render)
    .def("renderCubeMap", &SUNCGRenderAPI::renderCubeMap);


  py::class_<SUNCGRenderAPIThread>(m, "RenderAPIThread")
    // device defaults to 0
    .def(py::init<int, int, int>(), "Initialize", "w"_a, "h"_a, "device"_a=0)
    .def("getCamera", &SUNCGRenderAPIThread::getCamera, py::return_value_policy::reference)
    .def("printContextInfo", &SUNCGRenderAPIThread::printContextInfo)
    .def("setMode", &SUNCGRenderAPIThread::setMode)
    .def("loadSceneSUNCG", &SUNCGRenderAPIThread::loadScene)
    .def("loadScene", &SUNCGRenderAPIThread::loadScene)
    .def("resolution", &SUNCGRenderAPIThread::resolution)
    .def("render", &SUNCGRenderAPIThread::render)
    .def("renderCubeMap", &SUNCGRenderAPIThread::renderCubeMap);

  auto camera = py::class_<Camera>(m, "Camera")
    .def("shift", &Camera::shift)
    .def("turn", &Camera::turn)
    .def("updateDirection", &Camera::updateDirection)
    .def_readwrite("pos", &Camera::pos)
    .def_readwrite("yaw", &Camera::yaw) // init yaw = -90 --> facing (0,0,1)
    .def_readwrite("pitch", &Camera::pitch) // init pitch = 0
    .def_readwrite("near", &Camera::near) // init near = 0.1
    .def_readwrite("far", &Camera::far) // init pitch = 100
    .def_readwrite("vertical_fov", &Camera::vertical_fov) // init fov = 60
    .def_readonly("front", &Camera::front)  // init front = (0,0,1)
    .def_readonly("right", &Camera::right)
    .def_readonly("up", &Camera::up);

  py::class_<Geometry>(m, "Geometry")
    .def_readonly("w", &Geometry::w)
    .def_readonly("h", &Geometry::h);

  py::enum_<SUNCGScene::RenderMode>(m, "RenderMode")
    .value("RGB", SUNCGScene::RenderMode::RGB)
    .value("SEMANTIC", SUNCGScene::RenderMode::SEMANTIC)
    .value("DEPTH", SUNCGScene::RenderMode::DEPTH)
    .value("INSTANCE", SUNCGScene::RenderMode::INSTANCE)
    .export_values();

  py::enum_<Camera::Movement>(camera, "Movement")
    .value("Forward", Camera::Movement::FORWARD)
    .value("Backward", Camera::Movement::BACKWARD)
    .value("Left", Camera::Movement::LEFT)
    .value("Right", Camera::Movement::RIGHT)
    .value("Up", Camera::Movement::UP)
    .value("Down", Camera::Movement::DOWN)
    .export_values();

  py::class_<House>(m, "_House")
    .def("f", &House::f);

  py::class_<glm::vec3>(m, "Vec3")
    .def(py::init<float, float, float>())
    .def(py::self + py::self)
    .def(py::self - py::self)
    .def(py::self += py::self)
    .def(py::self -= py::self)
    .def(float() * py::self)
    .def(py::self * float())
    .def(py::self / float())
    .def(py::self * py::self)
    .def(-py::self)
    .def("__str__", [](glm::vec3* v) {
          return ssprintf("[%f, %f, %f]", v->x, v->y, v->z);
        })
    .def_readwrite("x", &glm::vec3::x)
    .def_readwrite("y", &glm::vec3::y)
    .def_readwrite("z", &glm::vec3::z);


  py::class_<Matuc>(m, "Mat", py::buffer_protocol()).def_buffer([](Matuc &m) -> py::buffer_info {
      return py::buffer_info(m.ptr(),
          sizeof(unsigned char),
          py::format_descriptor<unsigned char>::format(),
          3,
          {(unsigned long)m.rows(), (unsigned long)m.cols(),
          (unsigned long)m.channels()},
          {sizeof(unsigned char) * m.cols() * m.channels(),
          sizeof(unsigned char) * m.channels(), sizeof(unsigned char)});
      });
}
