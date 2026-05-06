#include <iostream>
#include <string>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>
#include <opencv2/video/background_segm.hpp>


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
	std::string str;
	std::string str1;
	int key = 0;

	/* Leitura de v�deo de um ficheiro */
	/* NOTA IMPORTANTE:
	O ficheiro video.avi dever� estar localizado no mesmo direct�rio que o ficheiro de c�digo fonte.
	*/
	capture.open(videofile);

	/* Em alternativa, abrir captura de v�deo pela Webcam #0 */
	//capture.open(0, cv::CAP_DSHOW); // Pode-se utilizar apenas capture.open(0);

	/* Verifica se foi poss�vel abrir o ficheiro de v�deo */
	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de v�deo!\n";
		return 1;
	}

	/* N�mero total de frames no v�deo */
	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	/* Frame rate do v�deo */
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	/* Resolu��o do v�deo */
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	/* Cria uma janela para exibir o v�deo */
	cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);

	cv::Mat frame;


	// Imagens IVC

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

	// Variaveis para contagem total de laranjas
	int total_laranjas = 0;
	int prev_xc[64] = { 0 };
	int prev_yc[64] = { 0 };
	int prev_count = 0;

	while (key != 'q') {
		/* Leitura de uma frame do vídeo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Copia pixeis do frame para a imagem IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);
		// Altera a ordem dos pixeis de BGR para RGB
		vc_bgr_to_rgb(image, rgb);
		// Converte a imagem RGB para HSV
		vc_rgb_to_hsv(rgb, hsv);
		// Segmenta a imagem HSV para a cor laranja
		vc_hsv_segmentation(hsv, temps[1], 15, 35, 33, 100, 25, 100);
		// Fecho morfologico para eliminar ruido
		vc_binary_close(temps[1], temps[2], 3, 3);

		int nlabels = 0;
		OVC* blobs = vc_binary_blob_labelling(temps[2], temps[3], &nlabels);
		OVC* filtered_blobs = NULL;

		if (blobs != NULL) {
			vc_binary_blob_info(temps[3], blobs, nlabels);

			filtered_blobs = vc_blob_filter_area(blobs, &nlabels, 90000);
			free(blobs);
			blobs = NULL;

			if (filtered_blobs != NULL) {
				// Classifica cada laranja: calibre, deformacao, categoria EEC 379/71
				vc_orange_classify(filtered_blobs, nlabels);

				// Desenha bounding boxes (azul) e centroide (vermelho) na imagem IVC
				vc_draw_blobs(image, filtered_blobs, nlabels,
					0, 0, 255,   // bounding box
					255, 0, 0,   // centroide
					3);
			}
		}

		// Conta laranjas novas nesta frame e acumula no total
		if (filtered_blobs != NULL)
			vc_count_new_oranges(filtered_blobs, nlabels, prev_xc, prev_yc, &prev_count, &total_laranjas);
		else
			vc_count_new_oranges(NULL, 0, prev_xc, prev_yc, &prev_count, &total_laranjas);

		// Copia imagem IVC (com bounding boxes) de volta para o frame OpenCV
		memcpy(frame.data, image->data, video.width * video.height * 3);

		// Texto: numero de laranjas na frame atual
		str1 = "Laranjas frame: " + std::to_string(nlabels);
		cv::putText(frame, str1, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);

		// Texto: total acumulado de laranjas desde o inicio do video
		str1 = "Total: " + std::to_string(total_laranjas);
		cv::putText(frame, str1, cv::Point(20, 65), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

		// Texto por laranja: classificacao EEC 379/71
		if (filtered_blobs != NULL) {
			for (int j = 0; j < nlabels; j++) {
				//int tx = filtered_blobs[j].x;
				//int ty = filtered_blobs[j].y - 8;
				//if (ty < 40) ty = filtered_blobs[j].y + filtered_blobs[j].height + 20;

				int tx = filtered_blobs[j].x;
				int ty = filtered_blobs[j].y + filtered_blobs[j].height + 15;

				char buf1[128], buf2[128];
				snprintf(buf1, sizeof(buf1), "L:%d A:%d P:%d",
					filtered_blobs[j].label,
					filtered_blobs[j].area,
					filtered_blobs[j].perimeter);
				snprintf(buf2, sizeof(buf2), "CAL:%d D:%.0fmm DEF:%.1f%% CAT:%s",
					filtered_blobs[j].calibre,
					(double)filtered_blobs[j].diameter_mm,
					(double)filtered_blobs[j].deformation,
					vc_orange_category_name(filtered_blobs[j].category));

				cv::putText(frame, buf1, cv::Point(tx, ty),
					cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 0, 0), 1);
				cv::putText(frame, buf2, cv::Point(tx, ty + 18),
					cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 0, 0), 1);
			}
			free(filtered_blobs);
			filtered_blobs = NULL;
		}

		/* Exibe a frame */
		cv::imshow("VC - VIDEO", frame);

		/* Sai da aplicação, se o utilizador premir a tecla 'q' */
		key = cv::waitKey(1);
	}
	vc_image_free(image);

	/* Fecha a janela */
	cv::destroyWindow("VC - VIDEO");

	/* Fecha o ficheiro de v�deo */
	capture.release();

	return 0;
}