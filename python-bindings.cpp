#include <filesystem>

#include "pybind11/pybind11.h"
#include "pybind11/stl.h" // for std::map
#include "pybind11/stl/filesystem.h"

#include "pybind11_opencv.hpp"
#include "mimetrik/FacebowFileReader.hpp"

namespace py = pybind11;

PYBIND11_MODULE(FacebowFileReader, m)
{
    m.doc() = "Facebow MFBA file reader Python bindings";

    py::class_<mimetrik::FacebowFileReader>(m, "FacebowFileReader")
        .def(py::init<const std::filesystem::path&>(),
             "Construct a FacebowFileReader object for the given MFBA file.")
        .def("get_image_count", &mimetrik::FacebowFileReader::get_image_count,
             "Returns the number of images in the MFBA file.")
        .def("get_image", &mimetrik::FacebowFileReader::get_image, "Doc")
        .def("get_metadata", &mimetrik::FacebowFileReader::get_metadata, "Doc");
}
