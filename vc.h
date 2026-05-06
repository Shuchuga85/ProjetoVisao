//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Bin�rio/Cinzentos=1; RGB=3
	int levels;				// Bin�rio=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// �rea
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Per�metro
	int label;					// Etiqueta
	// Classificacao EEC 379/71 (preenchida por vc_orange_classify)
	float diameter_mm;			// Diametro equivalente em mm (calculado da bounding box)
	int calibre;				// Calibre EEC 0..13; -1 se abaixo do minimo (53 mm)
	float deformation;			// Deformacao (%) em relacao ao circulo ideal
	int category;				// 0=Extra 1=I 2=II 3=III; -1=nao classificado
} OVC;

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT�TIPOS DE FUN��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);
int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC *srcdst);
int vc_rgb_get_red_gray(IVC* srcdst);
int vc_rgb_get_green_gray(IVC* srcdst);
int vc_rgb_get_blue_gray(IVC* srcdst);
int vc_rgb_to_gray(IVC* src, IVC* dst);
int vc_bgr_to_rgb(IVC* src, IVC* dst);
int vc_rgb_to_hsv(IVC* src, IVC* dst);
int vc_hsv_segmentation(IVC* src, IVC* dst, int min_h, int max_h, int min_s, int max_s, int min_v, int max_v);
int vc_count_white_pixels(IVC* src);
int vc_scale_gray_to_color_palette(IVC* src, IVC* dst);
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);
int vc_gray_to_binary_range(IVC* src, IVC* dst, int min_threshold, int max_threshold);
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst);
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);
int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin);
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k_value);
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);
int vc_binary_erode(IVC* src, IVC* dst, int kernel);
int vc_binary_open(IVC* src, IVC* dst, int erode_kernel, int dilate_kernel);
int vc_binary_close(IVC* src, IVC* dst, int dilate_kernel, int erode_kernel);
int vc_mask_crop(IVC* src, IVC* mask, IVC* dst);
//int vc_binary_blob_labelling(IVC* src, IVC* dst);
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);
//int vc_draw_blobs(IVC* binary, OVC* blobs, int nblobs);
int vc_draw_blobs(IVC* binary,OVC* blobs,int nblobs,unsigned char sq_r, unsigned char sq_g, unsigned char sq_b,unsigned char cr_r, unsigned char cr_g, unsigned char cr_b,int thickness);
int vc_gray_histogram_equalization(IVC* src, IVC* dst);
int vc_gray_histogram_show(IVC* src, IVC* dst);
int vc_gray_edge_gradient(IVC* src, IVC* dst, int* mx, int* my, float th);
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);
int vc_gray_edge_sobel(IVC* src, IVC* dst, float th);
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize);
int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize);
int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst);
int vc_gray_to_rgb(IVC* src, IVC* dst);
OVC* vc_blob_filter_area(OVC* blobs, int* nblobs, int min_area);
OVC* vc_blob_filter_median(OVC* blobs, int* nblobs);
OVC* vc_blob_filter_mean(OVC* blobs, int* nblobs);
int vc_orange_classify(OVC* blobs, int nblobs);
const char* vc_orange_category_name(int category);
int vc_count_new_oranges(OVC* blobs, int nlabels, int* prev_xc, int* prev_yc, int* prev_count, int* total);
