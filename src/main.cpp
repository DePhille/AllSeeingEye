#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <chrono>

#include <PiEye/PiEye.h>
#include <PiEye/Log.hpp>

#include "CameraTuner.h"

using namespace std;

struct PixelData {
	uchar blue;
	uchar green;
	uchar red;
};

void initCamera(PiEye& camera) {
    camera.createCamera();
    camera.setSensorMode(SensorMode::V2_HD720P);
    camera.setEncoding(Encoding::NATIVE_GRAYSCALE);
    camera.setDigitalGain(1.0);
	camera.setWhiteBalanceGain(1.0, 1.0);
    camera.setWhiteBalanceMode(AwbMode::OFF);
}

std::vector<unsigned int> loadMask(const std::string& filename) {
	std::cout << "Reading mask image [" << filename << "]" << endl;
	std::vector<unsigned int> mask;
	cv::Mat maskImage = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
	if (!maskImage.data) {
		std::cout << "Unable to read mask image [" << filename << "]" << endl;
	} else {
		for (int row = 0; row < maskImage.rows; ++row) {
			uchar* rowData = maskImage.ptr<uchar>(row);
			for (int col = 0; col < maskImage.cols; ++col) {
				PixelData* pxlData = (PixelData*) (rowData + (col * 3));
				if (pxlData->blue == 0 && pxlData->green == 255 && pxlData->red == 0) {
					mask.push_back((row * maskImage.cols) + col);
				}
			}
		}
	}
	
	std::cout << "Found [" << mask.size() << "] pixels in mask [" << filename << "]" << std::endl;
	return mask;
}

unsigned int
detectMovement(const cv::Mat& prevImage, const cv::Mat& curImage, cv::Mat& diffImage, const std::vector<unsigned int> mask, unsigned short maxDiff) {
	if (!prevImage.isContinuous() || !curImage.isContinuous()) {
		std::cout << "Image not continuous, fuck it" << std::endl;
		return 0;
	}
	
	// Analyze img
	unsigned int diffPixels = 0;
	uchar* diffImageData = diffImage.ptr<uchar>(0);
	const uchar* prevImageData = prevImage.ptr<uchar>(0);
	const uchar* curImageData = curImage.ptr<uchar>(0);
	unsigned int curPixel;
	for (unsigned int i = 0; i < mask.size(); ++i) {
		curPixel = mask[i];
		diffImageData[curPixel] = abs(prevImageData[curPixel] - curImageData[curPixel]);
		if (diffImageData[curPixel] > maxDiff) {
			diffPixels++;
		}
	}
	
	// Put number of diffpixels on img:
	rectangle(diffImage, cv::Point(0,0), cv::Point(300, 40), cv::Scalar(0, 0, 0), CV_FILLED);
	putText(diffImage, "DiffPixels: " + std::to_string(diffPixels), cv::Point(15,15), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(255));
	
	return diffPixels;
}

int main(int argc, char* argv[]) {
    std::cout << "Opencv version: " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << std::endl;
    
	unsigned short detectionInterval = 1000; // ms
	unsigned short maxDiff = 40; // Intensity
	unsigned short maxDiffPixels = 300;
    PiEye camera;
    initCamera(camera);
	
	cv::Mat image1;
    cv::Mat image2;
	cv::Mat diffImage(720, 1280, CV_8UC1);
	cv::Mat* prevImage = &image1;
	cv::Mat* curImage = &image2;
	cv::Mat* tmpImage = nullptr;
	CameraTuner camTuner;
	unsigned int movementCounter = 0;
	
	// Load mask:
	std::vector<unsigned int> mask = loadMask("mask.bmp");
    
    // Tune camera for the first time
	camTuner.tune(camera, mask);
    
    // Start capture
    while (true) {
		EZLOG_INFO("Start loop------------------------------------------------------------");
		
		// Start timer
		const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        
        // Detect movement first
		camera.grabStill(*curImage);
        bool hasMovement = detectMovement(*prevImage, *curImage, diffImage, mask, maxDiff) > maxDiffPixels;
        
        // If no movement, maybe tune camera
        if (!hasMovement && camTuner.needsTuning(*curImage, mask)) {
			camTuner.tune(camera, mask);
			
			// Overwrite previous image to prevent movement detection caused by intensity change
			camera.grabStill(*prevImage);
        } else {
			// Swap image buffers
			tmpImage = prevImage;
			prevImage = curImage;
			curImage = tmpImage;
		}
		
		// Timer end
		const std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
		unsigned int millis = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
		EZLOG_INFO("Logic completed in [" << millis << "] msec");
		
		// Store if movement
		if (hasMovement) {
			movementCounter++;
			EZLOG_INFO("Movement [" << movementCounter << "] was detected!");
			cv::imwrite("still_" + std::to_string(movementCounter) + ".jpg", *curImage);
			cv::imwrite("movement_" + std::to_string(movementCounter) + ".jpg", diffImage);
		}
		
		// Sleep
		if (millis < detectionInterval) {
			unsigned short remainingTime = detectionInterval - millis;
			EZLOG_DEBUG("Sleeping for [" << remainingTime << "] ms");
			std::this_thread::sleep_for(std::chrono::milliseconds(remainingTime));
		}
    }
	
	// Stop
    cout << "Stop camera..." << endl;
    camera.destroyCamera();
	return 0;
}
