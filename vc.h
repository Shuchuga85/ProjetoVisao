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
	int   dark_spot_area;
	float spot_ratio;
} OVC;

/* Pixel-to-mm conversion calibration: 280 px = 55 mm (assignment specification) */
#define VC_PX_TO_MM  (55.0f / 280.0f)

/* HSV segmentation range for orange colour */
#define VC_ORANGE_H_MIN   15
#define VC_ORANGE_H_MAX   35
#define VC_ORANGE_S_MIN   33
#define VC_ORANGE_S_MAX  100
#define VC_ORANGE_V_MIN   25
#define VC_ORANGE_V_MAX  100

/* Dark spot detection tuning */
#define VC_SPOT_KERNEL  11
#define VC_SPOT_CMIN    30

/* Dark spot classification thresholds (% of orange surface, EEC 379/71 II.D) */
#define VC_SPOT_EXTRA   0.5f
#define VC_SPOT_I       1.0f
#define VC_SPOT_II      2.5f

/* Dark spot detection: histogram-mode approach.
   Pixels whose gray value falls more than VC_SPOT_MARGIN below the dominant
   orange-surface gray level (histogram mode) are marked as dark spots. */
#define VC_SPOT_MARGIN  25

/* Dark spot detection: local mean subtraction via integral image.
   Kernel should be larger than spots (~15px) but smaller than the shadow
   gradient (~100px). Threshold is the minimum residual to call a spot. */
#define VC_SPOT_KERNEL_LOCAL  31
#define VC_SPOT_THRESHOLD     8

/* Deformation thresholds (%) for quality category assignment (EEC 379/71) */
#define VC_DEFORM_EXTRA   5.0f
#define VC_DEFORM_I      10.0f
#define VC_DEFORM_II     20.0f

/* Blob filter thresholds derived from EEC 379/71 */
#define VC_MIN_DIAMETER_MM    53.0f   /* oranges below this diameter are excluded */
#define VC_FILTER_DEFORM_MAX  VC_DEFORM_II  /* blobs above this deformation % are excluded */

/* Orange tracking across frames */
#define VC_MAX_BLOBS  64
#define VC_DIST_MAX   80

/* Uniformity in size tolerance (EEC 379/71 Section V):
   maximum allowed diameter difference between smallest and largest orange */
#define VC_UNIFORM_TOL_LARGE   10.0f  /* mm — calibres 1–8  (diameter >= 73 mm) */
#define VC_UNIFORM_TOL_SMALL    7.0f  /* mm — calibres 9–13 (diameter 53–73 mm) */
#define VC_UNIFORM_BOUNDARY_MM 73.0f  /* mm — boundary between the two calibre groups */

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
int vc_binary_dilate_fast(IVC* src, IVC* dst, int kernel);
int vc_binary_erode_fast(IVC* src, IVC* dst, int kernel);
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
OVC* vc_blob_filter_deformation(OVC* blobs, int* nblobs);
int vc_orange_classify(OVC* blobs, int nblobs);
const char* vc_orange_category_name(int category);
int vc_count_new_oranges(OVC* blobs, int nlabels, int* prev_xc, int* prev_yc, int* prev_count, int* total);
int vc_gray_local_mean_detect(IVC* masked_gray, IVC* dst, int kernel, int threshold);
int vc_gray_local_mean_detect_fast(IVC* img, IVC* dst, int kernel, int threshold);
int vc_blob_count_dark_spots(IVC* spot_binary, IVC* label_image, OVC* blobs, int nblobs);
int vc_binary_subtract(IVC* src, IVC* subtrahend, IVC* dst);