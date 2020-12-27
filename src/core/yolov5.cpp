#include "yolov5.h"

YOLOv5::YOLOv5(const string &config_file) : Detection(config_file){}

YOLOv5::~YOLOv5(){}

float desigmoid(float sigmoid_value) {
	return -log(1 / sigmoid_value - 1);
}

float sigmoid(float value) {
	return 1 / (1 + exp(-value));
}

struct Anchor {
	int width[3], height[3];
};

static Rect decodeBox_yolov5(float dx, float dy, float dw, float dh, float cellx, float celly, int stride, int anchorWidth, int anchorHeight, Size netSize) {

	float cx = (dx * 2 - 0.5f + cellx) * stride;
	float cy = (dy * 2 - 0.5f + celly) * stride;
	float w = pow(dw * 2, 2) * anchorWidth;
	float h = pow(dh * 2, 2) * anchorHeight;
	float x = (cx - w * 0.5f);
	float y = (cy - h * 0.5f);
	float r = (cx + w * 0.5f);
	float b = (cy + h * 0.5f);

	return Rect(Point(x, y), Point(r + 1, b + 1));
}

void decode_yolov5(const shared_ptr<TRTInfer::Tensor>& tensor, int stride, float threshold, int num_classes,
	const vector<pair<float, float>>& anchors, vector<ccutil::BBox> &bboxs, Size netInputSize) {
	int batch = tensor->num();
	int tensor_channel = tensor->channel();
	int tensor_width = tensor->width();
	int tensor_height = tensor->height();
	int area = tensor_width * tensor_height;
	Anchor anchor;
	for (int i = 0; i < anchors.size(); ++i) {
		anchor.width[i] = anchors[i].first;
		anchor.height[i] = anchors[i].second;
	}

	float threshold_desigmoid = desigmoid(threshold);
	for (int conf_channel = 0; conf_channel < anchors.size(); conf_channel++) {
		int conf_offset = 4 + conf_channel * (num_classes + 5);
		for (int i = 0; i < tensor_height; ++i) {
			for (int j = 0; j < tensor_width; ++j) {
				int inner_offset = i * tensor_height + j;
				float* ptr = tensor->cpu<float>(0, conf_offset) + inner_offset;
				if (*ptr < threshold_desigmoid) {
					continue;
				}
				float obj_confidence = sigmoid(*ptr);
				float* pclasses = ptr + area;
				float max_class_confidence = *pclasses;
				int max_classes = 0;
				pclasses += area;
				for (int k = 1; k < num_classes; ++k, pclasses += area) {
					if (*pclasses > max_class_confidence) {
						max_classes = k;
						max_class_confidence = *pclasses;
					}
				}
				max_class_confidence = sigmoid(max_class_confidence) * obj_confidence;
				if (max_class_confidence < threshold)
					continue;
				float* pbbox = ptr - 4 * area;
				float dx = sigmoid(*pbbox);  pbbox += area;
				float dy = sigmoid(*pbbox);  pbbox += area;
				float dw = sigmoid(*pbbox);  pbbox += area;
				float dh = sigmoid(*pbbox);  pbbox += area;

				ccutil::BBox box = decodeBox_yolov5(dx, dy, dw, dh, j, i, stride, anchor.width[conf_channel], anchor.height[conf_channel], netInputSize);
				box.label = max_classes;
				box.score = max_class_confidence;
				if (box.area() > 0)
					bboxs.push_back(box);
			}
		}
	}
}

vector<ccutil::BBox> YOLOv5::EngineInference(const Mat& image) {

	if (engine_ == nullptr) {
		INFO("EngineInference failure, model is nullptr");
		return vector<ccutil::BBox>();
	}
	engine_->input()->resize(1);
	Size netInputSize = engine_->input()->size();
	Size imageSize = image.size();
	preprocessImageToTensor(image, 0, engine_->input());

	engine_->forward();
	auto output1 = engine_->output(2);
	auto output2 = engine_->output(1);
	auto output3 = engine_->output(0);
	vector<ccutil::BBox> bboxs;

	decode_yolov5(output1, strides_[2], obj_threshold_, num_classes_, anchor_grid_32, bboxs, netInputSize);
	decode_yolov5(output2, strides_[1], obj_threshold_, num_classes_, anchor_grid_16, bboxs, netInputSize);
	decode_yolov5(output3, strides_[0], obj_threshold_, num_classes_, anchor_grid_8, bboxs, netInputSize);
	auto& objs = bboxs;
	objs = ccutil::nms(objs, nms_threshold_);
	outPutBox(objs, imageSize, netInputSize);

	return bboxs;
}

vector<vector<ccutil::BBox>> YOLOv5::EngineInferenceOptim(const vector<Mat>& images) {

	if (engine_ == nullptr) {
		INFO("EngineInference failure call, model is nullptr");
		return vector<vector<ccutil::BBox>>();
	}

	engine_->input()->resize(images.size());
	Size netInputSize = engine_->input()->size();
	vector<Size> imagesSize;

	for (int i = 0; i < images.size(); i++) {
		preprocessImageToTensor(images[i], i, engine_->input());
		imagesSize.emplace_back(images[i].size());
	}
	engine_->forward(false);
	auto output1 = engine_->output(2);
	auto output2 = engine_->output(1);
	auto output3 = engine_->output(0);
	TRTInfer::YOLOv5DetectBackend detectBackend(obj_threshold_, num_classes_, max_objs_, engine_->getCUStream());
	vector<vector<ccutil::BBox>> bboxs = detectBackend.forwardGPU(output1, output2, output3, imagesSize, netInputSize);
	for (int i = 0; i < bboxs.size(); ++i) {
		auto& objs = bboxs[i];
		objs = ccutil::nms(objs, nms_threshold_);
		outPutBox(objs, imagesSize[i], netInputSize);
	}
	return bboxs;
}
