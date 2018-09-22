#include "CameraTuner.h"

#include <PiEye/PiEye.h>
#include <PiEye/Log.hpp>
#include "Utils.hpp"

CameraTuner::CameraTuner() {
	_options.push_back({1, {1, 2, 3, 5, 7, 10, 15}});
	_options.push_back({2, {7, 10, 15, 20, 25, 30, 40}});
	_options.push_back({4, {30, 40, 50, 66, 75, 100}});
	_options.push_back({8, {66, 75, 100, 150, 200, 300, 500, 666, 750, 1000}});
}

CameraTuner::~CameraTuner() {
	
}

bool
CameraTuner::needsTuning(const cv::Mat& image, const std::vector<unsigned int>& mask) {
	const unsigned short avgIntensity = calcAvgIntensity(image, mask);
	return avgIntensity < _minIntensity || avgIntensity > _maxIntensity;
}

bool
CameraTuner::tune(PiEye& camera, const std::vector<unsigned int>& mask) {
	EZLOG_INFO("Tuning camera");
	
	// Loop
	cv::Mat probeImg;
	bool tuned = false;
	do {
		unsigned short analogGain = _options.at(_analogPos).analogGain;
		unsigned short shutterSpeed = _options.at(_analogPos).shutterSpeeds.at(_shutterPos);
		EZLOG_TRACE("Setting analog gain [" << analogGain << "] and shutter speed [" << shutterSpeed << "]");
		camera.setAnalogGain(analogGain);
		camera.setShutterSpeed(shutterSpeed);
		
		// Get image from camera
		EZLOG_TRACE("Waiting for camera to stabilize");
		for (unsigned short i = 0; i < _stabilizationImages; ++i) {
			camera.grabStill(probeImg);
		}
		camera.grabStill(probeImg);
		
		// Analyze intensity
		unsigned short avgIntensity = calcAvgIntensity(probeImg, mask);
		EZLOG_DEBUG("Found intensity [" << avgIntensity << "] for analog gain [" << analogGain << "] and shutter speed [" << shutterSpeed << "]");
		if (avgIntensity < _minIntensity) {
			EZLOG_TRACE("Intensity [" << avgIntensity << "] too low");
			tuned = false;
			
			_shutterPos++;
			if (_shutterPos >= _options.at(_analogPos).shutterSpeeds.size()) {
				_shutterPos = 0;
				_analogPos++;
				if (_analogPos >= _options.size()) {
					EZLOG_INFO("Reached maximum shutter option");
					return false;
				}
			}
		} else if (avgIntensity > _maxIntensity) {
			EZLOG_TRACE("Intensity [" << avgIntensity << "] too high");
			tuned = false;
			if (_shutterPos > 0) {
				_shutterPos--;
			} else if (_analogPos > 0) {
				_analogPos--;
				_shutterPos = _options.at(_analogPos).shutterSpeeds.size() - 1;
			} else {
				EZLOG_INFO("Reached minimum shutter option");
				return false;
			}
		} else {
			tuned = true;
		}
	} while(!tuned);
	
	return tuned;
}
