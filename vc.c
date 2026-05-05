//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}

int vc_gray_negative(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <=0 )|| (srcdst->height <=0) || (srcdst->data == NULL))
	{
		return 0;
	}
	if (channels != 1 )
	{
		return 0;
	}


	for (y = 0; y < height; y++)
	{
		for ( x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

#pragma region Vc_rgb



int vc_rgb_negative(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;


	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
	{
		return 0;
	}
	if (channels != 3)
	{
		return 0;
	}

	for ( y = 0; y < height; y++)
	{
		for ( x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			
			data[pos] = 255 - data[pos];
			data[pos + 1] = 255 - data[pos + 1];
			data[pos + 2] = 255 - data[pos + 2];
		}
	}

	return 1;
}



int vc_rgb_get_red_gray(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;


	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
	{
		return 0;
	}
	if (channels != 3)
	{
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos + 1] = data[pos];
			data[pos + 2] = data[pos];
		}
	}

	return 1;
}


int vc_rgb_get_green_gray(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;


	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
	{
		return 0;
	}
	if (channels != 3)
	{
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos+1];
			data[pos + 2] = data[pos+1];
		}
	}

	return 1;
}


int vc_rgb_get_blue_gray(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;


	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
	{
		return 0;
	}
	if (channels != 3)
	{
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos+2];
			data[pos + 1] = data[pos+2];
		}
	}

	return 1;
}


int vc_rgb_to_gray(IVC* src, IVC* dst) {

	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int width = src->width;
	int height = src->height;
	
	int x, y;
	long int pos_src,pos_dst;
	float rf, gf, bf;
	


	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
	{
		return 0;
	}
	if ((src->width != dst->width) || (src->height != dst->height))
	{
		return 0;
	}
	if ((src->channels != 3)||(dst->channels != 1))
	{
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src +1];
			bf = (float)datasrc[pos_src +2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));

		}
	}

	return 1;
}

int vc_bgr_to_rgb(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int width = src->width;
	int height = src->height;

	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((src->channels != 3) || (dst->channels != 3))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			// Swap B and R channels
			datadst[pos_dst] = datasrc[pos_src + 2]; // R
			datadst[pos_dst + 1] = datasrc[pos_src + 1]; // G
			datadst[pos_dst + 2] = datasrc[pos_src];     // B
		}
	}

	return 1;
}

int vc_rgb_to_hsv(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;

	int width = src->width;
	int height = src->height;

	int x, y;
	long int pos_src, pos_dst;

	float r, g, b;
	float max, min, delta;
	float h, s, v;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
	{
		return 0;
	}
	if ((src->width != dst->width) || (src->height != dst->height))
	{
		return 0;
	}
	if ((src->channels != 3) || (dst->channels != 3))
	{
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			r = datasrc[pos_src] / 255.0f;
			g = datasrc[pos_src + 1] / 255.0f;
			b = datasrc[pos_src + 2] / 255.0f;

			max = r;
			if (g > max) max = g;
			if (b > max) max = b;

			min = r;
			if (g < min) min = g;
			if (b < min) min = b;

			delta = max - min;

			v = max;

			if (max == 0.0f)
				s = 0.0f;
			else
				s = delta / max;


			if (delta == 0.0f)
			{
				h = 0.0f;
			}
			else
			{
				/*
				if (max == r)
					h = 60.0f * fmodf(((g - b) / delta), 6.0f);
				else if (max == g)
					h = 60.0f * (((b - r) / delta) + 2.0f);
				else
					h = 60.0f * (((r - g) / delta) + 4.0f);

				if (h < 0.0f)
					h += 360.0f;
				*/

				if (max == g)
				{
					h = 120.0f + (60.0f * (b - r) / delta);
				}
				else if (max  == b)
				{
					h = 240.0f + (60.0f * (r - g) / delta);
				}
				else
				{
					if (g>=b)
					{
						h = (60.0f * (g - b) / delta);
					}
					else
					{
						h = 360.0f + (60.0f * (g - b) / delta);
						if (h>360.0f)
						{
							h = h - 360.0f;
						}
					}
				}
			}


			//                                  percentage  * bytesyze
			datadst[pos_dst] = (unsigned char)((h / 360.0f) * 255.0f);
			datadst[pos_dst + 1] = (unsigned char)(s * 255.0f);
			datadst[pos_dst + 2] = (unsigned char)(v * 255.0f);
		}
	}

	return 1;
}
#pragma endregion


int vc_hsv_segmentation(IVC* src, IVC* dst,int min_h, int max_h, int min_s, int max_s, int min_v, int max_v) {

	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;

	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;


	int width = src->width;
	int height = src->height;

	int isinvert = (min_h > max_h) ? 1 : 0;

	max_h = (int)((max_h / 360.0f) * 255.0f);
	min_h = (int)((min_h / 360.0f) * 255.0f);

	max_s = (int)((max_s / 100.0f) * 255.0f);
	min_s = (int)((min_s / 100.0f) * 255.0f);

	max_v = (int)((max_v / 100.0f) * 255.0f);
	min_v = (int)((min_v / 100.0f) * 255.0f);

	int x, y;
	long int pos_dst, pos_src;
	float h, s, v;


	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
	{
		return 0;
	}
	if ((src->width != dst->width) || (src->height != dst->height))
	{
		return 0;
	}
	if ((src->channels != 3) || (dst->channels != 1))
	{
		return 0;
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			h = (float)datasrc[pos_src];
			s = (float)datasrc[pos_src + 1];
			v = (float)datasrc[pos_src + 2];

			if (isinvert == 0)
			{
				if (h >= min_h && h <= max_h &&
					s >= min_s && s <= max_s &&
					v >= min_v && v <= max_v)
				{
					datadst[pos_dst] = 255;
				}
				else
				{
					datadst[pos_dst] = 0;
				}
			}
			else
			{
				if (((h >= 0 && h <= max_h)||(h >= min_h && h <= 255)) &&
					s >= min_s && s <= max_s &&
					v >= min_v && v <= max_v)
				{
					datadst[pos_dst] = 255;
				}
				else
				{
					datadst[pos_dst] = 0;
				}
			}
			
		}
	}

	return 1;
}


int vc_count_white_pixels(IVC* src)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	int count = 0;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;

	if (channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			if (data[pos] == 255)
			{
				count++;
			}
		}
	}

	return count;
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int x, y;
	long int pos_src, pos_dst;
	int v;
	unsigned char r, g, b;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 3))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			v = datasrc[pos_src];

			if (v < 64)
			{
				r = 0;
				g = v * 4;
				b = 255;
			}
			else if (v < 128)
			{
				r = 0;
				g = 255;
				b = 255 - ((v - 64) * 4);
			}
			else if (v < 192)
			{
				r = (v - 128) * 4;
				g = 255;
				b = 0;
			}
			else
			{
				r = 255;
				g = 255 - ((v - 192) * 4);
				b = 0;
			}

			datadst[pos_dst] = r;
			datadst[pos_dst + 1] = g;
			datadst[pos_dst + 2] = b;
		}
	}

	return 1;
}


#pragma region vc_gray_to_binary



int vc_gray_to_binary(IVC* src, IVC* dst, int threshold)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int white = dst->levels * 1;

	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}


int vc_gray_to_binary_range(IVC* src, IVC* dst, int min_threshold, int max_threshold)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int white = dst->levels * 1;

	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] >= min_threshold && datasrc[pos_src] <= max_threshold)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_global_mean(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;

	int x, y;
	long int pos_src, pos_dst;
	int white = dst->levels * 1;


	long sum = 0;
	int threshold;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			sum += datasrc[pos_src];
		}
	}

	threshold = sum / (width * height);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;

	int white = dst->levels * 1;
	int x, y, i, j;
	long int pos_src, pos_dst, pos_k;

	int k = kernel / 2; 
	int vmin, vmax;
	int threshold;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			vmin = 255;
			vmax = 0;

			for (j = -k; j <= k; j++)
			{
				for (i = -k; i <= k; i++)
				{
					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline_src + xx * channels_src;

						if (datasrc[pos_k] < vmin)
							vmin = datasrc[pos_k];

						if (datasrc[pos_k] > vmax)
							vmax = datasrc[pos_k];
					}
				}
			}

			threshold = (vmin + vmax) / 2;

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;

	int x, y, i, j;
	long int pos_src, pos_dst, pos_k;
	int white = dst->levels * 1;


	int k = kernel / 2;
	int vmin, vmax;
	int threshold;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			vmin = 255;
			vmax = 0;

			for (j = -k; j <= k; j++)
			{
				for (i = -k; i <= k; i++)
				{
					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline_src + xx * channels_src;

						if (datasrc[pos_k] < vmin) vmin = datasrc[pos_k];
						if (datasrc[pos_k] > vmax) vmax = datasrc[pos_k];
					}
				}
			}

			if ((vmax - vmin) < cmin)
				threshold = 127; 
			else
				threshold = (vmin + vmax) / 2;

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}



int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k_value)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;

	int x, y, i, j;
	long int pos_src, pos_dst, pos_k;
	int white = dst->levels * 1;


	int k = kernel / 2;

	float sum, sum_sq;
	float mean, stddev;
	float threshold;

	int count;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0.0f;
			sum_sq = 0.0f;
			count = 0;

			for (j = -k; j <= k; j++)
			{
				for (i = -k; i <= k; i++)
				{
					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline_src + xx;

						float val = (float)datasrc[pos_k];

						sum += val;
						sum_sq += val * val;
						count++;
					}
				}
			}

			mean = sum / count;

			stddev = sqrt((sum_sq / count) - (mean * mean));

			threshold = mean + k_value * stddev;

			pos_src = y * bytesperline_src + x;
			pos_dst = y * bytesperline_dst + x;

			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}

#pragma endregion

int vc_binary_dilate(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;

	int x, y, i, j;
	long int pos_dst, pos_k;
	int white = dst->levels * 1;

	int k = kernel / 2;
	int found;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			found = 0;

			for (j = -k; j <= k && !found; j++)
			{
				for (i = -k; i <= k; i++)
				{
					if ((i * i + j * j) > (k * k))
						continue;

					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline_src + xx;

						if (datasrc[pos_k] == 255)
						{
							found = 1;
							break;
						}
					}
				}
			}

			pos_dst = y * bytesperline_dst + x;

			if (found)
				datadst[pos_dst] = white;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_dst = width * channels_dst;

	int x, y, i, j;
	long int pos_dst, pos_k;
	int white = dst->levels * 1;


	int k = kernel / 2;
	int erode;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y * bytesperline_dst + x;

			if (datasrc[y * bytesperline_src + x] == 0)
			{
				datadst[pos_dst] = 0;
				continue;
			}

			erode = 0;

			for (j = -k; j <= k && !erode; j++)
			{
				for (i = -k; i <= k; i++)
				{
					if ((i * i + j * j) > (k * k))
						continue;

					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline_src + xx;

						if (datasrc[pos_k] == 0)
						{
							erode = 1;
							break;
						}
					}
				}
			}

			datadst[pos_dst] = erode ? 0 : white;
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int erode_kernel, int dilate_kernel)
{
	IVC* temp;

	if ((src == NULL) || (dst == NULL))
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	temp = vc_image_new(src->width, src->height, 1, 255);
	if (temp == NULL)
		return 0;

	vc_binary_erode(src, temp, erode_kernel);

	vc_binary_dilate(temp, dst, dilate_kernel);

	vc_image_free(temp);

	return 1;
}

int vc_binary_close(IVC* src, IVC* dst, int dilate_kernel, int erode_kernel)
{
	IVC* temp;

	if ((src == NULL) || (dst == NULL))
		return 0;

	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;

	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	temp = vc_image_new(src->width, src->height, 1, 255);
	if (temp == NULL)
		return 0;

	vc_binary_dilate(src, temp, dilate_kernel);

	vc_binary_erode(temp, dst, erode_kernel);

	vc_image_free(temp);

	return 1;
}

int vc_mask_crop(IVC* src, IVC* mask, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datamask = (unsigned char*)mask->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_mask = mask->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = width * channels_src;
	int bytesperline_mask = width * channels_mask;
	int bytesperline_dst = width * channels_dst;

	int x, y;
	long int pos_src, pos_mask, pos_dst;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != mask->width) || (height != mask->height))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((channels_src != 1) || (channels_mask != 1) || (channels_dst != 1))
		return 0;

	// Process
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x;
			pos_mask = y * bytesperline_mask + x;
			pos_dst = y * bytesperline_dst + x;

			if (datamask[pos_mask] == 255)
				datadst[pos_dst] = datasrc[pos_src]; // keep
			else
				datadst[pos_dst] = 0; // remove
		}
	}

	return 1;
}




int old_vc_binary_blob_labelling(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = width;

	int x, y;
	long int pos;

	int label = 1;
	
	int neighbours[4];
	int i, min;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((width != dst->width) || (height != dst->height))
		return 0;

	if ((src->channels != 1) || (dst->channels != 1))
		return 0;


	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x;

			if (datasrc[pos] == 255)
			{
				// Get neighbours A, B, C, D
				neighbours[0] = (x > 0 && y > 0) ? datadst[(y - 1) * bytesperline + (x - 1)] : 0; // A
				neighbours[1] = (y > 0) ? datadst[(y - 1) * bytesperline + x] : 0;               // B
				neighbours[2] = (x < width - 1 && y > 0) ? datadst[(y - 1) * bytesperline + (x + 1)] : 0; // C
				neighbours[3] = (x > 0) ? datadst[y * bytesperline + (x - 1)] : 0;               // D

				// Find minimum label (excluding 0)
				min = 0;
				for (i = 0; i < 4; i++)
				{
					if (neighbours[i] != 0)
					{
						if (min == 0 || neighbours[i] < min)
							min = neighbours[i];
					}
				}

				if (min == 0)
				{
					datadst[pos] = label;
					label++;
				}
				else
				{
					datadst[pos] = min;
				}
			}
			else
			{
				datadst[pos] = 0;
			}
		}
	}

	return label - 1; 
}

// Etiquetagem de blobs
// src		: Imagem bin�ria de entrada
// dst		: Imagem grayscale (ir� conter as etiquetas)
// nlabels	: Endere�o de mem�ria de uma vari�vel, onde ser� armazenado o n�mero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. � necess�rio libertar posteriormente esta mem�ria.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	/* Label values are stored in a single byte (0–255), so max 254 distinct blobs. */
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem bin�ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
	// Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
	// Ser�o atribu�das etiquetas no intervalo [1,254]
	// Este algoritmo est� assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin�ria
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A est� marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do n�mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se n�o h� blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta �rea de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// �rea
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Per�metro
					// Se pelo menos um dos quatro vizinhos n�o pertence ao mesmo label, ent�o � um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / (blobs[i].area > 0 ? blobs[i].area : 1);
		blobs[i].yc = sumy / (blobs[i].area > 0 ? blobs[i].area : 1);
	}

	return 1;
}

int vc_draw_blobs(
	IVC* binary,
	OVC* blobs,
	int nblobs,
	unsigned char sq_r, unsigned char sq_g, unsigned char sq_b,
	unsigned char cr_r, unsigned char cr_g, unsigned char cr_b,
	int thickness
)
{
	unsigned char* data = (unsigned char*)binary->data;

	int width = binary->width;
	int height = binary->height;
	int bytesperline = binary->bytesperline;
	int channels = binary->channels;

	int i, x, y, t;
	long int pos;

	if ((binary == NULL) || (blobs == NULL)) return 0;
	if (thickness < 1) thickness = 1;

	for (i = 0; i < nblobs; i++)
	{
		int xmin = blobs[i].x;
		int ymin = blobs[i].y;
		int xmax = xmin + blobs[i].width - 1;
		int ymax = ymin + blobs[i].height - 1;

		int xc = blobs[i].xc;
		int yc = blobs[i].yc;

		// ----------- DRAW RECTANGLE -----------
		for (t = 0; t < thickness; t++)
		{
			// Top & Bottom
			for (x = xmin; x <= xmax; x++)
			{
				if (ymin + t >= 0 && ymin + t < height && x >= 0 && x < width)
				{
					pos = (ymin + t) * bytesperline + x * channels;
					if (channels == 1)
						data[pos] = (sq_r > 127) ? 255 : 0;
					else
					{
						data[pos] = sq_r;
						data[pos + 1] = sq_g;
						data[pos + 2] = sq_b;
					}
				}

				if (ymax - t >= 0 && ymax - t < height && x >= 0 && x < width)
				{
					pos = (ymax - t) * bytesperline + x * channels;
					if (channels == 1)
						data[pos] = (sq_r > 127) ? 255 : 0;
					else
					{
						data[pos] = sq_r;
						data[pos + 1] = sq_g;
						data[pos + 2] = sq_b;
					}
				}
			}

			// Left & Right
			for (y = ymin; y <= ymax; y++)
			{
				if (xmin + t >= 0 && xmin + t < width && y >= 0 && y < height)
				{
					pos = y * bytesperline + (xmin + t) * channels;
					if (channels == 1)
						data[pos] = (sq_r > 127) ? 255 : 0;
					else
					{
						data[pos] = sq_r;
						data[pos + 1] = sq_g;
						data[pos + 2] = sq_b;
					}
				}

				if (xmax - t >= 0 && xmax - t < width && y >= 0 && y < height)
				{
					pos = y * bytesperline + (xmax - t) * channels;
					if (channels == 1)
						data[pos] = (sq_r > 127) ? 255 : 0;
					else
					{
						data[pos] = sq_r;
						data[pos + 1] = sq_g;
						data[pos + 2] = sq_b;
					}
				}
			}
		}

		// ----------- DRAW CROSS -----------
		int size = 5;

		for (t = -thickness / 2; t <= thickness / 2; t++)
		{
			// Horizontal
			for (x = xc - size; x <= xc + size; x++)
			{
				if (x >= 0 && x < width && yc + t >= 0 && yc + t < height)
				{
					pos = (yc + t) * bytesperline + x * channels;
					if (channels == 1)
						data[pos] = (cr_r > 127) ? 255 : 0;
					else
					{
						data[pos] = cr_r;
						data[pos + 1] = cr_g;
						data[pos + 2] = cr_b;
					}
				}
			}

			// Vertical
			for (y = yc - size; y <= yc + size; y++)
			{
				if (y >= 0 && y < height && xc + t >= 0 && xc + t < width)
				{
					pos = y * bytesperline + (xc + t) * channels;
					if (channels == 1)
						data[pos] = (cr_r > 127) ? 255 : 0;
					else
					{
						data[pos] = cr_r;
						data[pos + 1] = cr_g;
						data[pos + 2] = cr_b;
					}
				}
			}
		}
	}

	return 1;
}

int vc_gray_histogram_equalization(IVC* src, IVC* dst)
{
	int x, y;
	int width, height;
	long int pos;
	unsigned char* datasrc, * datadst;

	int hist[256] = { 0 };
	float cdf[256] = { 0 };
	int total_pixels;
	float cdf_min = 0;

	if ((src->width <= 0) || (src->height <= 0) ||
		(src->data == NULL) || (src->channels != 1))
		return 0;

	datasrc = src->data;
	datadst = dst->data;
	width = src->width;
	height = src->height;
	total_pixels = width * height;

	int bytesperline = src->bytesperline;

	// ---------------------------
	// 1. Histogram
	// ---------------------------
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x;
			hist[datasrc[pos]]++;
		}
	}

	// ---------------------------
	// 2. CDF (cumulative)
	// ---------------------------
	cdf[0] = (float)hist[0];
	for (int i = 1; i < 256; i++)
	{
		cdf[i] = cdf[i - 1] + hist[i];
	}

	// ---------------------------
	// 3. Normalize CDF
	// ---------------------------
	for (int i = 0; i < 256; i++)
	{
		cdf[i] /= total_pixels;
	}

	// ---------------------------
	// 4. Find first non-zero cdf
	// ---------------------------
	for (int i = 0; i < 256; i++)
	{
		if (cdf[i] > 0)
		{
			cdf_min = cdf[i];
			break;
		}
	}

	// ---------------------------
	// 5. Apply transformation
	// ---------------------------
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x;

			float value = (cdf[datasrc[pos]] - cdf_min) / (1.0f - cdf_min);
			if (value < 0) value = 0;

			datadst[pos] = (unsigned char)(value * 255.0f);
		}
	}

	return 1;
}

int vc_gray_histogram_show(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	int channels_src = src->channels;
	int channels_dst = dst->channels;

	int bytesperline_src = src->bytesperline;
	int bytesperline_dst = dst->bytesperline;

	int ni[256] = { 0 };
	float pdf[256] = { 0 };
	float pdfnorm[256] = { 0 };
	float pdfmax = 0.0f;

	int n = width * height;

	int x, y;
	long int pos;

	// Validation
	if ((width <= 0) || (height <= 0) || (datasrc == NULL))
		return 0;

	if ((channels_src != 1) || (channels_dst != 1))
		return 0;

	// dst must be 256x256
	if ((dst->width != 256) || (dst->height != 256))
		return 0;

	// -------------------------
	// 1. Histogram
	// -------------------------
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x;
			ni[datasrc[pos]]++;
		}
	}

	// -------------------------
	// 2. PDF
	// -------------------------
	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)ni[i] / (float)n;

		if (pdf[i] > pdfmax)
			pdfmax = pdf[i];
	}

	// -------------------------
	// 3. Normalize
	// -------------------------
	for (int i = 0; i < 256; i++)
	{
		pdfnorm[i] = pdf[i] / pdfmax;
	}

	// -------------------------
	// 4. Clear output image
	// -------------------------
	for (int i = 0; i < 256 * 256; i++)
	{
		datadst[i] = 0;
	}

	// -------------------------
	// 5. Draw histogram
	// -------------------------
	for (x = 0; x < 256; x++)
	{
		for (y = 255; y >= (255 - (int)(pdfnorm[x] * 255)); y--)
		{
			datadst[y * 256 + x] = 255;
		}
	}

	return 1;
}

int vc_gray_edge_gradient(IVC* src, IVC* dst, int* mx, int* my, float th)
{
	int x, y, i, j;
	int width, height;
	long int pos;
	unsigned char* datasrc, * datadst;

	int gx, gy;
	float magnitude;
	int histogram[256] = { 0 };
	int total_pixels;
	int threshold = 0, sum = 0;

	if ((src->width <= 0) || (src->height <= 0) ||
		(src->data == NULL) || (src->channels != 1))
		return 0;

	datasrc = src->data;
	datadst = dst->data;
	width = src->width;
	height = src->height;

	// Temporary buffer for magnitudes
	float* mag = (float*)malloc(width * height * sizeof(float));
	if (mag == NULL) return 0;

	// ---- Compute gradient ----
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			gx = 0;
			gy = 0;

			for (j = -1; j <= 1; j++)
			{
				for (i = -1; i <= 1; i++)
				{
					pos = (y + j) * width + (x + i);
					gx += datasrc[pos] * mx[(j + 1) * 3 + (i + 1)];
					gy += datasrc[pos] * my[(j + 1) * 3 + (i + 1)];
				}
			}

			magnitude = (float)sqrt((gx * gx) + (gy * gy));
			if (magnitude > 255) magnitude = 255;

			mag[y * width + x] = magnitude;
			histogram[(int)magnitude]++;
		}
	}

	// ---- Compute threshold from histogram ----
	total_pixels = width * height;
	int target = (int)(total_pixels * th);

	for (i = 0; i < 256; i++)
	{
		sum += histogram[i];
		if (sum >= target)
		{
			threshold = i;
			break;
		}
	}

	// ---- Apply threshold ----
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			if (mag[y * width + x] >= threshold)
				datadst[y * width + x] = 255;
			else
				datadst[y * width + x] = 0;
		}
	}

	free(mag);
	return 1;
}

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th)
{
	int mx[9] = {
		-1, 0, 1,
		-1, 0, 1,
		-1, 0, 1
	};

	int my[9] = {
		-1, -1, -1,
		 0,  0,  0,
		 1,  1,  1
	};

	return vc_gray_edge_gradient(src, dst, mx, my, th);
}

int vc_gray_edge_sobel(IVC* src, IVC* dst, float th)
{
	int mx[9] = {
		-1, 0, 1,
		-2, 0, 2,
		-1, 0, 1
	};

	int my[9] = {
		-1, -2, -1,
		 0,  0,  0,
		 1,  2,  1
	};

	return vc_gray_edge_gradient(src, dst, mx, my, th);
}

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	int x, y, i, j;
	long int pos_dst, pos_k;
	int k = kernelsize / 2;
	long int sum;
	int count;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if ((channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0;
			count = 0;

			for (j = -k; j <= k; j++)
			{
				for (i = -k; i <= k; i++)
				{
					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline + xx;
						sum += datasrc[pos_k];
						count++;
					}
				}
			}

			pos_dst = y * bytesperline + x;
			datadst[pos_dst] = (unsigned char)(sum / count);
		}
	}

	return 1;
}

static int compare_uchar(const void* a, const void* b)
{
	return (*(unsigned char*)a - *(unsigned char*)b);
}

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	int x, y, i, j;
	long int pos_dst, pos_k;
	int k = kernelsize / 2;
	int count;
	unsigned char window[1024]; /* large enough for any practical kernel */

	if ((width <= 0) || (height <= 0) || (datasrc == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if ((channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			count = 0;

			for (j = -k; j <= k; j++)
			{
				for (i = -k; i <= k; i++)
				{
					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline + xx;
						window[count++] = datasrc[pos_k];
					}
				}
			}

			qsort(window, count, sizeof(unsigned char), compare_uchar);

			pos_dst = y * bytesperline + x;
			datadst[pos_dst] = window[count / 2];
		}
	}

	return 1;
}

int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	/* 5x5 discrete Gaussian kernel (sigma=1.0), normalisation factor 1/273 */
	static const int kernel[5][5] = {
		{ 1,  4,  7,  4,  1 },
		{ 4, 16, 26, 16,  4 },
		{ 7, 26, 41, 26,  7 },
		{ 4, 16, 26, 16,  4 },
		{ 1,  4,  7,  4,  1 }
	};

	int x, y, i, j;
	long int pos_dst, pos_k;
	long int sum;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if ((channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0;
			int weight_sum = 0;

			for (j = -2; j <= 2; j++)
			{
				for (i = -2; i <= 2; i++)
				{
					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						pos_k = yy * bytesperline + xx;
						int w = kernel[j + 2][i + 2];
						sum += datasrc[pos_k] * w;
						weight_sum += w;
					}
				}
			}

			pos_dst = y * bytesperline + x;
			datadst[pos_dst] = (unsigned char)(sum / weight_sum);
		}
	}

	return 1;
}

int vc_gray_to_rgb(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;

	if ((width <= 0) || (height <= 0) || (datasrc == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if (src->channels != 1 || dst->channels != 3) return 0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			unsigned char val = datasrc[y * width + x];
			int pos_dst = y * dst->bytesperline + x * 3;
			datadst[pos_dst]     = val;
			datadst[pos_dst + 1] = val;
			datadst[pos_dst + 2] = val;
		}
	}

	return 1;
}

OVC* vc_blob_filter_area(OVC* blobs, int* nblobs, int min_area)
{
	int i, count = 0;

	if (blobs == NULL || nblobs == NULL || *nblobs <= 0)
		return NULL;

	// Count valid blobs
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area >= min_area)
			count++;
	}

	if (count == 0)
	{
		*nblobs = 0;
		return NULL;
	}

	// Allocate new array
	OVC* filtered = (OVC*)calloc(count, sizeof(OVC));
	if (filtered == NULL) return NULL;

	// Copy valid blobs
	int j = 0;
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area >= min_area)
		{
			filtered[j++] = blobs[i];
		}
	}

	*nblobs = count;
	return filtered;
}

int compare_int(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

OVC* vc_blob_filter_median(OVC* blobs, int* nblobs)
{
	int i, count = 0;

	if (blobs == NULL || nblobs == NULL || *nblobs <= 0)
		return NULL;

	// ----------- Compute median -----------
	int* areas = (int*)malloc(sizeof(int) * (*nblobs));
	if (areas == NULL) return NULL;

	for (i = 0; i < *nblobs; i++)
		areas[i] = blobs[i].area;

	qsort(areas, *nblobs, sizeof(int), compare_int);

	int median;
	if (*nblobs % 2 == 0)
		median = (areas[*nblobs / 2 - 1] + areas[*nblobs / 2]) / 2;
	else
		median = areas[*nblobs / 2];

	free(areas);

	// ----------- Count valid blobs -----------
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area >= median)
			count++;
	}

	if (count == 0)
	{
		*nblobs = 0;
		return NULL;
	}

	// ----------- Allocate filtered array -----------
	OVC* filtered = (OVC*)calloc(count, sizeof(OVC));
	if (filtered == NULL) return NULL;

	int j = 0;
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area >= median)
		{
			filtered[j++] = blobs[i];
		}
	}

	*nblobs = count;
	return filtered;
}

int vc_blob_mean_area(OVC* blobs, int nblobs)
{
	if (blobs == NULL || nblobs <= 0) return 0;

	long int sum = 0;
	int i;

	for (i = 0; i < nblobs; i++)
	{
		sum += blobs[i].area;
	}

	return (int)(sum / nblobs);
}
OVC* vc_blob_filter_mean(OVC* blobs, int* nblobs)
{
	int i, count = 0;

	if (blobs == NULL || nblobs == NULL || *nblobs <= 0)
		return NULL;

	// ----------- Compute mean -----------
	int mean = vc_blob_mean_area(blobs, *nblobs);

	// ----------- Count valid blobs -----------
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area >= mean)
			count++;
	}

	if (count == 0)
	{
		*nblobs = 0;
		return NULL;
	}

	// ----------- Allocate filtered array -----------
	OVC* filtered = (OVC*)calloc(count, sizeof(OVC));
	if (filtered == NULL) return NULL;

	int j = 0;
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].area >= mean)
		{
			filtered[j++] = blobs[i];
		}
	}

	*nblobs = count;
	return filtered;
}

// ---------------------------------------------------------------------------
// Orange classification - EEC Regulation 379/71
// Calibration: 280 pixels = 55 mm (from assignment specification)
// ---------------------------------------------------------------------------

// EEC orange size table [calibre 0..13]: {min_mm, max_mm}
static const int vc_orange_size_table[14][2] = {
	{100, 999}, /* 0  */
	{ 87, 100}, /* 1  */
	{ 84,  96}, /* 2  */
	{ 81,  92}, /* 3  */
	{ 77,  88}, /* 4  */
	{ 73,  84}, /* 5  */
	{ 70,  80}, /* 6  */
	{ 67,  76}, /* 7  */
	{ 64,  73}, /* 8  */
	{ 62,  70}, /* 9  */
	{ 60,  68}, /* 10 */
	{ 58,  66}, /* 11 */
	{ 56,  63}, /* 12 */
	{ 53,  60}, /* 13 */
};

// Deformation thresholds (%) for quality category assignment.
// Based on: deform = |blob_area - ideal_circle_area| / ideal_circle_area * 100
// where ideal_circle_area = pi * (max(width,height)/2)^2
#define VC_DEFORM_EXTRA  0.35f
#define VC_DEFORM_I      0.65f
#define VC_DEFORM_II     1.00f

int vc_orange_classify(OVC* blobs, int nblobs)
{
	int i, c;
	const float px_to_mm = 55.0f / 280.0f;
	const float pi = 3.14159265f;

	if (blobs == NULL || nblobs <= 0) return 0;

	for (i = 0; i < nblobs; i++)
	{
		// ---- Diameter & Calibre ----
		float diam_px = (blobs[i].width + blobs[i].height) / 2.0f;
		blobs[i].diameter_mm = diam_px * px_to_mm;

		// Assign calibre by finding the range whose centre is closest to the
		// measured diameter. Requires the diameter to actually fall inside the range.
		blobs[i].calibre = -1;
		float best_dist = 1e9f;
		for (c = 0; c < 14; c++)
		{
			float lo = (float)vc_orange_size_table[c][0];
			float hi = (float)vc_orange_size_table[c][1];
			if (blobs[i].diameter_mm < lo || blobs[i].diameter_mm > hi)
				continue;
			float centre = (lo + hi) / 2.0f;
			float dist = blobs[i].diameter_mm - centre;
			if (dist < 0.0f) dist = -dist;
			if (dist < best_dist)
			{
				best_dist = dist;
				blobs[i].calibre = c;
			}
		}

		// ---- Shape deformation ----
		// Compares the blob's actual area to the area of an ideal circle whose
		// diameter equals the longest bounding-box dimension.
		float max_dim = (blobs[i].width > blobs[i].height)
			? (float)blobs[i].width : (float)blobs[i].height;
		float ideal_area = pi * (max_dim / 2.0f) * (max_dim / 2.0f);
		float diff = (float)blobs[i].area - ideal_area;
		if (diff < 0.0f) diff = -diff;
		blobs[i].deformation = (ideal_area > 0.0f)
			? (diff / ideal_area * 100.0f) : 0.0f;

		// ---- Quality Category (EEC 379/71 Section II.D) ----
		if (blobs[i].deformation < VC_DEFORM_EXTRA)
			blobs[i].category = 0; // Extra
		else if (blobs[i].deformation < VC_DEFORM_I)
			blobs[i].category = 1; // Class I
		else if (blobs[i].deformation < VC_DEFORM_II)
			blobs[i].category = 2; // Class II
		else
			blobs[i].category = 3; // Class III
	}

	return 1;
}

const char* vc_orange_category_name(int category)
{
	switch (category)
	{
	case 0:  return "Extra";
	case 1:  return "I";
	case 2:  return "II";
	case 3:  return "III";
	default: return "N/A";
	}
}