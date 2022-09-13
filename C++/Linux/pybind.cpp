// https://pybind11.readthedocs.io/en/stable/index.html


#include <pybind11/pybind11.h>
#include "PhoneCamera.h"
#include "mat_wrapper.h"


namespace py = pybind11;


class PyPhoneCamera {
public:
    explicit PyPhoneCamera(int port) { pc = new PhoneCamera(port); }
    ~PyPhoneCamera() { delete pc; }
    bool isOpened() const { return pc->isOpened(); }
    double get(int n) const { return pc->get((cv::VideoCaptureProperties)n); }
    py::tuple read() {
        cv::Mat image;
        (*pc) >> image;
        return py::make_tuple(pc->isOpened(), cv_mat_uint8_3c_to_numpy(image)); 
    };
    
private:
    PhoneCamera *pc;
};


PYBIND11_MODULE(phonecam, m) {
    m.doc() = "Python binding of the C++ PhoneCamera receiver. Receiving images from a phone camera in real-time.";
    py::class_<PyPhoneCamera>(m, "PhoneCamera")
        .def(py::init<int>(), "Start a PhoneCamera receiver at a specific port. \nThe constructor will be blocked till the connection is established.", py::arg("port"))
        .def("isOpened", &PyPhoneCamera::isOpened, "Whether the connection is opened.")
        .def("get", &PyPhoneCamera::get, "Get the timestamp in milliseconds with get(0). \nCurrently, only cv2.CAP_PROP_POS_MSEC (=0) is implemented.", py::arg("propId"))
        .def("read", &PyPhoneCamera::read, "Read the next image (may be blocked). \nReturn a tuple (succeed, image)");
}

