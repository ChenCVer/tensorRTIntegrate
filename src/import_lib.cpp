﻿

//导入OpenCV，根据编译情况选择不同库
#if defined(_DEBUG)
#	pragma comment(lib, "opencv_world346d.lib")
#else
#	pragma comment(lib, "opencv_world346.lib")
#endif

//导入yaml
#if defined(_DEBUG)
#	pragma comment(lib, "yaml-cppd.lib")
#else
#	pragma comment(lib, "yaml-cpp.lib")
#endif

//导入cuda
#pragma comment(lib, "cuda.lib")
#pragma comment(lib, "cudart.lib")
#pragma comment(lib, "cublas.lib")
#pragma comment(lib, "cudnn.lib")

//导入tensorRT
#pragma comment(lib, "nvinfer.lib")
#pragma comment(lib, "nvinfer_plugin.lib")
#pragma comment(lib, "nvparsers.lib")
//#pragma comment(lib, "nvonnxparser.lib")

#if defined(_DEBUG)
#pragma comment(lib, "libprotobufd.lib")
#else
#pragma comment(lib, "libprotobuf.lib")
#endif