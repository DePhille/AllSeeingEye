#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

unsigned short
calcAvgIntensity(const cv::Mat& image, const std::vector<unsigned int>& mask) {
	// Analyze img
	unsigned long totalPixel = 0;
	const uchar* imageData = image.ptr<uchar>(0);
	for (unsigned int i = 0; i < mask.size(); ++i) {
		totalPixel += imageData[mask[i]];
	}
	
	// Calculate average intensity
	const unsigned short avgIntensity = totalPixel / mask.size();
	
	return avgIntensity;
}
