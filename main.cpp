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
	video.fps =(int)capture.get(cv::CAP_PROP_FPS);
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


	while (key != 'q') {
		/* Leitura de uma frame do v�deo */
		capture.read(frame);

		/* Verifica se conseguiu ler a frame */
		if (frame.empty()) break;

		/* Número da frame a processar */
		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		// Adicionar texto com ao vídeo
		str = "RESOLUCAO: " + std::to_string(video.width) + "x" + std::to_string(video.height);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 0, 0), 1);

		// Faça o seu código aqui...
		
		// Copia pixeis do frame para a imagem IVC
		memcpy(image->data, frame.data, video.width * video.height * 3);
		// Altera a ordem dos pixeis de BGR para RGB
		vc_bgr_to_rgb(image, rgb);
		// Converte a imagem RGB para HSV
		vc_rgb_to_hsv(rgb, hsv);
		// Segmenta a imagem HSV, obtendo uma imagem binária
		vc_hsv_segmentation(hsv, temps[1], 15, 35, 50, 100, 28, 100);
		// Aplica operações morfológicas de fecho (dilatação seguida de erosão) para eliminar ruído
		vc_binary_close(temps[1], temps[2], 3,3);


		int nlabels;
		// Obtem a lista de blobs (objectos) da imagem binária, com respetiva etiquetagem
		OVC* blobs = vc_binary_blob_labelling(temps[2], temps[3], &nlabels);

		if (blobs != NULL) {
			// Imprime a informação de cada blob
			vc_binary_blob_info(temps[3], blobs, nlabels);
			// Desenha os blobs na imagem original
			vc_draw_blobs(image, blobs, nlabels,
				0, 0, 255, // cor da caixa delimitadora (bounding box)
				255, 0, 0, // cor do centro-de-massa
				3); // thickness = 3
			free(blobs);
		}


		// Copia os pixeis da imagem IVC para a frame
		memcpy(frame.data, image->data, video.width * video.height * 3);



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