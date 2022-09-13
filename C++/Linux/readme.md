Build the project using:

```
mkdir build
cd build
cmake ..
make
```

You will get:

- C++ library `libPhoneCamera.a` (header file `PhoneCamera.h` in the source directory)
- C++ executable example `record_video` (run using `./record_video`)
- python module `phonecam.*.so` (use by `import phonecam`)

The C++ example is in `record_video.cpp`. The python example is in `example.py`.  You need to copy `phonecam.*.so` from your build directory to run the python example. 

