#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>
#include <opencv2/video/background_segm.hpp>
#define DEBUG 0


extern "C" {
#include "vc.h"
}

int main(void) {
	char videofile[20] = "assets/video.avi";
	cv::VideoCapture capture;
	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;
	char key = 0;

	
	capture.open(videofile);

	
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de video!\n";
		return 1;
	}

	
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;

	IVC* hsv;
	IVC* rgb;
	enum { templength = 4 };
	IVC* temps[templength];


	hsv = vc_image_new(video.width, video.height, 3, 255);
	if (hsv == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 0;
	}

	rgb = vc_image_new(video.width, video.height, 3, 255);
	if (rgb == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 0;
	}


	for (int i = 0; i < templength; i++)
	{
		temps[i] = vc_image_new(video.width, video.height, 1, 255);
		if (temps[i] == NULL)
		{
			printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
			getchar();
			return 0;
		}
	}


	IVC* image = vc_image_new(video.width, video.height, 3, 255);

	IVC* gray = vc_image_new(video.width, video.height, 1, 255);
	if (gray == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 0;
	}

	IVC* masked_gray = vc_image_new(video.width, video.height, 1, 255);
	if (masked_gray == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 0;
	}

	IVC* spot_binary = vc_image_new(video.width, video.height, 1, 255);
	if (spot_binary == NULL)
	{
		printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
		getchar();
		return 0;
	}

	int total_laranjas = 0;
	int prev_xc[VC_MAX_BLOBS] = { 0 };
	int prev_yc[VC_MAX_BLOBS] = { 0 };
	int prev_count = 0;
	bool paused = false;

	float global_min_diam = 1e9f;
	float global_max_diam = 0.0f;
	cv::Scalar blobTextColor;
	if (DEBUG != 0)
		blobTextColor = cv::Scalar(255, 255, 255);
	else
		blobTextColor = cv::Scalar(0, 0, 0);


	while (key != 'q') {
		if (!paused)
		{
		capture.read(frame);

		if (frame.empty()) break;

		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		memcpy(image->data, frame.data, video.width * video.height * 3);


		vc_bgr_to_rgb(image, rgb);
		vc_rgb_to_hsv(rgb, hsv);
		vc_hsv_segmentation(hsv, temps[1],
			VC_ORANGE_H_MIN, VC_ORANGE_H_MAX,
			VC_ORANGE_S_MIN, VC_ORANGE_S_MAX,
			VC_ORANGE_V_MIN, VC_ORANGE_V_MAX);
		vc_binary_close(temps[1], temps[2], 3, 3);


		vc_rgb_to_gray(rgb, gray);
		vc_mask_crop(gray, temps[2], masked_gray);
		vc_gray_local_mean_detect_fast(masked_gray, spot_binary, VC_SPOT_KERNEL_LOCAL, VC_SPOT_THRESHOLD);

		int nlabels = 0;
		OVC* blobs = vc_binary_blob_labelling(temps[2], temps[3], &nlabels);
		OVC* filtered_blobs = NULL;

		if (blobs != NULL) {
			vc_binary_blob_info(temps[3], blobs, nlabels);

			vc_orange_classify(blobs, nlabels);

			filtered_blobs = vc_blob_filter_deformation(blobs, &nlabels);
			free(blobs);
			blobs = NULL;

			if (filtered_blobs != NULL) {
				vc_blob_count_dark_spots(spot_binary, temps[3], filtered_blobs, nlabels);

				vc_orange_classify(filtered_blobs, nlabels);

				vc_draw_blobs(image, filtered_blobs, nlabels,
					255, 0, 0,   
					255, 0, 0,   
					3);

				vc_draw_blobs(temps[2], filtered_blobs, nlabels,
					255, 255, 255,
					0, 0, 0,
					3);
			}
		}

		if (filtered_blobs != NULL && nlabels > 0) {
			for (int j = 0; j < nlabels; j++) {
				if (filtered_blobs[j].diameter_mm > global_max_diam) global_max_diam = filtered_blobs[j].diameter_mm;
				if (filtered_blobs[j].diameter_mm < global_min_diam) global_min_diam = filtered_blobs[j].diameter_mm;
			}
		}

		bool have_oranges = (global_max_diam > 0.0f);
		float global_size_diff = have_oranges ? (global_max_diam - global_min_diam) : 0.0f;


		float global_uniform_tol = (have_oranges && global_min_diam >= VC_UNIFORM_BOUNDARY_MM)
			? VC_UNIFORM_TOL_LARGE : VC_UNIFORM_TOL_SMALL;
		bool global_uniform_pass = !have_oranges || (global_size_diff <= global_uniform_tol);


		if (filtered_blobs != NULL)
			vc_count_new_oranges(filtered_blobs, nlabels, prev_xc, prev_yc, &prev_count, &total_laranjas);
		else
			vc_count_new_oranges(NULL, 0, prev_xc, prev_yc, &prev_count, &total_laranjas);




		if (DEBUG == 0)
		{
			memcpy(frame.data, image->data, video.width * video.height * 3);
		}
		else
		{
			memcpy(rgb->data, image->data, video.width * video.height * 3);
			switch(DEBUG)
			{
				case 1:
					vc_gray_to_rgb(temps[2], rgb);
					break;
				case 2:
					vc_gray_to_rgb(masked_gray, rgb);
					break;
				case 3:
					vc_gray_to_rgb(spot_binary, rgb);
					break;
				case 4:
					vc_binary_subtract(temps[2], spot_binary, temps[0]);
					vc_gray_to_rgb(temps[0], rgb);
					break;
			}
			memcpy(frame.data, rgb->data, video.width * video.height * 3);
		}


		const cv::Scalar INFO_COLOR(255, 255, 255);
		char buf_top[128];
		snprintf(buf_top, sizeof(buf_top), "Total: %d", total_laranjas);
		cv::putText(frame, buf_top, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.9, INFO_COLOR, 2);
		snprintf(buf_top, sizeof(buf_top), "Laranjas na frame: %d", nlabels);
		cv::putText(frame, buf_top, cv::Point(20, 62), cv::FONT_HERSHEY_SIMPLEX, 0.9, INFO_COLOR, 2);
		snprintf(buf_top, sizeof(buf_top), "Frame: %d / %d", video.nframe, video.ntotalframes);
		cv::putText(frame, buf_top, cv::Point(20, 94), cv::FONT_HERSHEY_SIMPLEX, 0.9, INFO_COLOR, 2);



		if (filtered_blobs != NULL) {
			const float px2mm2 = (float)(VC_PX_TO_MM * VC_PX_TO_MM);
			for (int j = 0; j < nlabels; j++) {
				int tx = filtered_blobs[j].x;
				int ty = filtered_blobs[j].y + filtered_blobs[j].height + 15;

				char buf1[128], buf2[128];
				snprintf(buf1, sizeof(buf1), "Label:%d Area:%.0fmm2 Perim:%.1fmm",
					filtered_blobs[j].label,
					filtered_blobs[j].area * px2mm2,
					filtered_blobs[j].perimeter * VC_PX_TO_MM);
				snprintf(buf2, sizeof(buf2), "CAL:%d Diam:%.0fmm DEF:%.1f%% SPOT:%.1f%% CAT:%s",
					filtered_blobs[j].calibre,
					(double)filtered_blobs[j].diameter_mm,
					(double)filtered_blobs[j].deformation,
					(double)filtered_blobs[j].spot_ratio,
					vc_orange_category_name(filtered_blobs[j].category));

				cv::putText(frame, buf1, cv::Point(tx, ty),
					cv::FONT_HERSHEY_SIMPLEX, 0.6, blobTextColor, 2);
				cv::putText(frame, buf2, cv::Point(tx, ty + 18),
					cv::FONT_HERSHEY_SIMPLEX, 0.6, blobTextColor, 2);
			}
			free(filtered_blobs);
			filtered_blobs = NULL;
		}

		

		{
			char buf_bot[256];
			int by = video.height - 40;
			if (have_oranges) {
				snprintf(buf_bot, sizeof(buf_bot), "Diferenca maxima de tamanho: %.1fmm", (double)global_size_diff);
				cv::putText(frame, buf_bot, cv::Point(20, by),
					cv::FONT_HERSHEY_SIMPLEX, 0.7, INFO_COLOR, 2);
				snprintf(buf_bot, sizeof(buf_bot), "Uniformidade em tamanho: %s  (lim: %.0fmm)",
					global_uniform_pass ? "CONFORME" : "NAO CONFORME", (double)global_uniform_tol);
				cv::putText(frame, buf_bot, cv::Point(20, by + 30),
					cv::FONT_HERSHEY_SIMPLEX, 0.7,
					global_uniform_pass ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2);
			} else {
				cv::putText(frame, "Uniformidade: N/A", cv::Point(20, by),
					cv::FONT_HERSHEY_SIMPLEX, 0.7, INFO_COLOR, 2);
			}
		}

		cv::imshow("VC - VIDEO", frame);
	}



		key = cv::waitKey(1);
		if (key == 'p')
		{
			paused = true;
		}
		else if (key == 'c')
		{
			paused = false;
		}
	}
	vc_image_free(image);
	vc_image_free(gray);
	vc_image_free(masked_gray);
	vc_image_free(spot_binary);
	vc_image_free(hsv);
	vc_image_free(rgb);
	for (int i = 0; i < templength; i++)
		vc_image_free(temps[i]);

	cv::destroyWindow("VC - VIDEO");

	capture.release();

	printf("\n=======================================\n");
	printf("       RELATORIO FINAL DE ANALISE      \n");
	printf("=======================================\n");
	printf("Frames processadas    : %d / %d\n", video.nframe, video.ntotalframes);
	printf("Total de laranjas     : %d\n", total_laranjas);
	if (global_max_diam > 0.0f) {
		float diff = global_max_diam - global_min_diam;
		float tol  = (global_min_diam >= VC_UNIFORM_BOUNDARY_MM)
		             ? VC_UNIFORM_TOL_LARGE : VC_UNIFORM_TOL_SMALL;
		printf("Diametro minimo       : %.1f mm\n", (double)global_min_diam);
		printf("Diametro maximo       : %.1f mm\n", (double)global_max_diam);
		printf("Diferenca de tamanho  : %.1f mm\n", (double)diff);
		printf("Limite uniformidade   : %.0f mm  (EEC 379/71)\n", (double)tol);
		printf("Uniformidade          : %s\n", (diff <= tol) ? "CONFORME" : "NAO CONFORME");
	} else {
		printf("Nenhuma laranja detetada.\n");
	}
	printf("=======================================\n");

	return 0;
}