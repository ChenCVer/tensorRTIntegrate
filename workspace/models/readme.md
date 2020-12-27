# **yolov5**模型导出

Tensort7.0.0.11, ONNX模型创建网络和预测都需要用V2版本：

```c++
//创建模型
const auto explicitBatch = 1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    INetworkDefinition *network = builder->createNetworkV2(explicitBatch);

//预测模型
context_->enqueueV2(buffer_queue_.data(), stream_, nullptr);
```

创建的时候需要创建优化配置类对象，根据输入尺寸设置为动态：

```c++
IBuilderConfig *builder_config = builder->createBuilderConfig();
IOptimizationProfile *profile = builder->createOptimizationProfile();
ITensor *input = network->getInput(0);
Dims dims = input->getDimensions();
profile->setDimensions(input->getName(), 
                       OptProfileSelector::kMIN, Dims4{1, dims.d[1], dims.d[2], dims.d[3]});
//max_batch_size_根据自己模型实际情况设置
profile->setDimensions(input->getName(),
                           OptProfileSelector::kOPT, Dims4{max_batch_size_, dims.d[1], dims.d[2], dims.d[3]});
profile->setDimensions(input->getName(),
                           OptProfileSelector::kMAX, Dims4{max_batch_size_, dims.d[1], dims.d[2], dims.d[3]});
builder_config->addOptimizationProfile(profile);
//创建cuda_engine 用带withconfig去创建
ICudaEngine *engine = builder->buildEngineWithConfig(*network, *builder_config);
```

预测之前需要绑定维度，因为pt转onnx时设置的动态batch_size（转换流程见后面）：

```c++
//对应参数是batchsize  channel height width   这里只有batchsize是浮动的，其他三个就是网络的输出尺寸
Dims4 input_dims{batch_size_, input_tensor_.channel(), input_tensor_.height(), input_tensor_.width()};
context_->setBindingDimensions(0, input_dims);
context_->enqueueV2(buffer_queue_.data(), stream_, nullptr);
```

注意初始化内部变量时候inputdim.d[0]由于是占位符号，值变成-1，这里用设置最大max_batch_size。输出维度信息也一样：

```c++
Dims inputdim = engine_->getBindingDimensions(b); // C*H*W
input_shape_.Reshape(max_batch_size_, inputdim.d[1], inputdim.d[2], inputdim.d[3]); // [batch_size, C, H, W]
```

转模型时动态的使用batch_size，Tensort7.0.0.11,设置 opset_version=10，在[yolov5](https://github.com/ultralytics/yolov5)/models/  修改yolo.py

copy export_onnx.py into `yolov5/models` and run `export_onnx.py` to generate `yolov5s.onnx` and so on.

导出形式是三个尺度的输出

**output1[batch_size,255,80,80]**

**output2[batch_size,255,40,40]**

**output3[batch_size,255,20,20]**

转好的onnx模型，提取码：[qnzv](https://pan.baidu.com/s/1prK97E8O0polwqDZg_JULQ ) 