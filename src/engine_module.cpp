#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "blunderfish.h"
#include "position.cpp"
#include "common.h"

namespace py = pybind11;

PYBIND11_MODULE(engine, m) {
    py::class_<Side>(m, "Side")
    .def(py::init<>())
    .def("can_castle_kingside", &Side::can_castle_kingside)
    .def("can_castle_queenside", &Side::can_castle_queenside)
    .def_property_readonly("bb", &Side::get_bbs);

    py::class_<Position>(m, "Position")
    .def(py::init<>())
    .def_static("decode_fen_string", &Position::decode_fen_string)
    .def("display", &Position::display)
    .def_readonly("to_move", &Position::to_move)
    .def_readonly("piece_at", &Position::piece_at)
    .def_property_readonly("sides", &Position::get_sides);
}