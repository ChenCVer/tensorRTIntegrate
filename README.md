日志

- YOLOv3-SPP、YOLOv4检测流程同YOLOv5，仅修改config即可
- 添加yaml读取配置文件

- YOLOv5直接使用版


=======================================================================

=======================================================================


# TensorRT-Integrate

1. Support pytorch onnx plugin（DCN、HSwish ... etc.）
2. Simpler inference and plugin APIs

<br/>


## Re-implement
##### [CenterNet : ctdet_coco_dla_2x](https://github.com/xingyizhou/CenterNet)

![image1](workspace/results/1.centernet.coco2x.dcn.jpg)

<br/>

##### [CenterTrack: coco_tracking](https://github.com/xingyizhou/CenterTrack)

![coco.tracking.jpg](workspace/results/coco.tracking.jpg)

* [coco_tracking.onnx download](http://zifuture.com:1000/fs/public_models/coco_tracking.onnx)

* [nuScenes_3Dtracking.onnx download](http://zifuture.com:1000/fs/public_models/nuScenes_3Dtracking.onnx)

<br/>

##### [DBFace](https://github.com/dlunion/DBFace)

![selfie.draw.jpg](workspace/results/selfie.draw.jpg)



## Use TensorRT-Integrate

install protobuf == 3.11.4 (or >= 3.8.x, But it's more troublesome)

```bash
bash scripts/getALL.sh
make run -j32
```

<br/>

## Inference Code

```
auto engine = TRTInfer::loadEngine("models/efficientnet-b0.fp32.trtmodel");
float mean[3] = {0.485, 0.456, 0.406};
float std[3] = {0.229, 0.224, 0.225};
Mat image = imread("img.jpg");
auto input = engine->input();

// multi batch sample
input->resize(2);
input->setNormMatGPU(0, image, mean, std);
input->setNormMatGPU(1, image, mean, std);

engine->forward();

// get result and copy to cpu
engine->output(0)->cpu<float>();
engine->tensor("hm")->cpu<float>();
```

<br/>

## Environment

* tensorRT7.0 or tensorRT6.0
* opencv3.4.6
* cudnn7.6.3
* cuda10.0
* protobuf v3.8.x
* Visual Studio 2017
* [lean-windows.zip (include tensorRT、opencv、cudnn、cuda、protobuf)](http://zifuture.com:1000/fs/25.shared/lean.zip)

<br/>

## Plugin

1. Pytorch export ONNX:  [plugin_onnx_export.py](plugin_onnx_export.py)
2. [MReLU.cu](src/onnxplugin/plugins/MReLU.cu) 、[HSwish.cu](src/onnxplugin/plugins/HSwish.cu)、[DCNv2.cu](src/onnxplugin/plugins/DCNv2.cu)

<br/>
