1. Download from https://github.com/pybind/pybind11
2. Copy all files into here.

- Comment out line 28-47 in file "include/pybind11/stl.h",
which related to the std::optional and std::variant,
then disable PYBIND11_HAS_OPTIONAL and PYBIND11_HAS_VARIANT.

#define PYBIND11_HAS_OPTIONAL 0
#define PYBIND11_HAS_VARIANT 0