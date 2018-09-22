#pragma once

#include <vector>

class PiEye;

namespace cv {
	class Mat;
}

struct ExposureOption {
	unsigned short analogGain;
    std::vector<unsigned short> shutterSpeeds;
};

class CameraTuner {
public:
	CameraTuner();
	~CameraTuner();
	
	bool
	needsTuning(const cv::Mat& image, const std::vector<unsigned int>& mask);
	
	bool
	tune(PiEye& camera, const std::vector<unsigned int>& mask);
	
private:
	std::vector<ExposureOption> _options;
	unsigned short _analogPos = 0;
	unsigned short _shutterPos = 0;
	unsigned short _stabilizationImages = 2;
	unsigned short _minIntensity = 118;
	unsigned short _maxIntensity = 138;
};
