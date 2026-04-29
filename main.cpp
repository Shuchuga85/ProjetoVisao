#include <iostream>
#include <string>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video/background_segm.hpp>

extern "C" {
#include "vc.h"
}

// ------------------------------------------------------------
// Sample from professor (rename to main to run)
// ------------------------------------------------------------
int mainSample(void)
{
	char videofile[] = "assets/video.avi";
	cv::VideoCapture capture;

	struct {
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	std::string str;
	int key = 0;

	capture.open(videofile);

	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir o ficheiro de video!\n";
		return 1;
	}

	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	video.fps          = (int)capture.get(cv::CAP_PROP_FPS);
	video.width        = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height       = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;
	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		cv::imshow("VC - VIDEO", frame);
		key = cv::waitKey(1);
	}

	cv::destroyWindow("VC - VIDEO");
	capture.release();
	return 0;
}

// ------------------------------------------------------------
// Exercise 1: Motion detection - consecutive frame difference
// rename to main to run
// ------------------------------------------------------------
int mainFrameDiff(void)
{
	cv::VideoCapture capture("assets/video.avi");
	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir video!\n";
		return 1;
	}

	int w = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	int h = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	IVC *src      = vc_image_new(w, h, 3, 255);
	IVC *gray     = vc_image_new(w, h, 1, 255);
	IVC *prevGray = vc_image_new(w, h, 1, 255);
	IVC *diff     = vc_image_new(w, h, 1, 255);

	cv::namedWindow("Frame Diff", cv::WINDOW_AUTOSIZE);
	cv::Mat frame;
	cv::Mat display(h, w, CV_8UC1);
	int key = 0;
	int n = w * h;

	// seed previous frame
	capture.read(frame);
	memcpy(src->data, frame.data, w * h * 3);
	vc_rgb_to_gray(src, prevGray);

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		memcpy(src->data, frame.data, w * h * 3);
		vc_rgb_to_gray(src, gray);

		// absolute difference + threshold (motion if change > 25)
		for (int i = 0; i < n; i++) {
			int d = abs((int)gray->data[i] - (int)prevGray->data[i]);
			diff->data[i] = (d > 25) ? 255 : 0;
		}

		memcpy(prevGray->data, gray->data, n);
		memcpy(display.data, diff->data, n);

		cv::imshow("Frame Diff", display);
		key = cv::waitKey(1);
	}

	vc_image_free(src);
	vc_image_free(gray);
	vc_image_free(prevGray);
	vc_image_free(diff);
	cv::destroyWindow("Frame Diff");
	capture.release();
	return 0;
}

// ------------------------------------------------------------
// Exercise 2: Background subtraction with fixed background
// Background updates only when < 1% of pixels change > 16
// rename to main to run
// ------------------------------------------------------------
int mainBgSub(void)
{
	cv::VideoCapture capture("assets/video.avi");
	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir video!\n";
		return 1;
	}

	int w = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	int h = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	// background stored in color: formula uses argmax over R,G,B channels
	IVC *src        = vc_image_new(w, h, 3, 255);
	IVC *background = vc_image_new(w, h, 3, 255);
	IVC *diff       = vc_image_new(w, h, 1, 255);

	cv::namedWindow("BG Subtraction", cv::WINDOW_AUTOSIZE);
	cv::Mat frame;
	cv::Mat display(h, w, CV_8UC1);
	int key = 0;
	int n = w * h;
	int motionLimit = (int)(n * 0.01); // 1% of total pixels

	// initialise background with first frame
	capture.read(frame);
	memcpy(src->data, frame.data, w * h * 3);
	memcpy(background->data, src->data, w * h * 3);

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		memcpy(src->data, frame.data, w * h * 3);

		// M(x) = 1 if argmax_{R,G,B} |I(x) - B(x)| > T  (slide formula)
		int motionPixels = 0;
		for (int i = 0; i < n; i++) {
			int pos = i * 3;
			int d0 = abs((int)src->data[pos]     - (int)background->data[pos]);
			int d1 = abs((int)src->data[pos + 1] - (int)background->data[pos + 1]);
			int d2 = abs((int)src->data[pos + 2] - (int)background->data[pos + 2]);
			int dmax = d0 > d1 ? (d0 > d2 ? d0 : d2) : (d1 > d2 ? d1 : d2);
			diff->data[i] = (dmax > 16) ? 255 : 0;
			if (dmax > 16) motionPixels++;
		}

		// no movement: update background
		if (motionPixels < motionLimit)
			memcpy(background->data, src->data, w * h * 3);

		memcpy(display.data, diff->data, n);

		cv::imshow("BG Subtraction", display);
		key = cv::waitKey(1);
	}

	vc_image_free(src);
	vc_image_free(background);
	vc_image_free(diff);
	cv::destroyWindow("BG Subtraction");
	capture.release();
	return 0;
}

// ------------------------------------------------------------
// Exercise 3: Background subtraction with MOG2
// rename to main to run
// ------------------------------------------------------------
int mainMOG2(void)
{
	cv::VideoCapture capture("assets/video.avi");
	if (!capture.isOpened()) {
		std::cerr << "Erro ao abrir video!\n";
		return 1;
	}

	cv::namedWindow("MOG2", cv::WINDOW_AUTOSIZE);

	cv::Ptr<cv::BackgroundSubtractor> pMOG2 = cv::createBackgroundSubtractorMOG2();

	cv::Mat frame, fgMask;
	int key = 0;

	while (key != 'q') {
		capture.read(frame);
		if (frame.empty()) break;

		pMOG2->apply(frame, fgMask);

		cv::imshow("MOG2", fgMask);
		key = cv::waitKey(1);
	}

	cv::destroyWindow("MOG2");
	capture.release();
	return 0;
}

// ------------------------------------------------------------
// Active entry point — rename whichever exercise you want to run
// ------------------------------------------------------------
int main(void)
{
	return mainMOG2();
}
