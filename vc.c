
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"


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

	vc_binary_dilate_fast(src, temp, dilate_kernel);
	vc_binary_erode_fast(temp, dst, erode_kernel);

	vc_image_free(temp);
	return 1;
}

int vc_binary_dilate_fast(IVC* src, IVC* dst, int kernel)
{
	if (src == NULL || dst == NULL) return 0;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bpl_src = src->bytesperline;
	int bpl_dst = dst->bytesperline;
	int k = kernel / 2;
	int x, y, lo, hi;
	int prefixlen;
	int* hpass;
	int* prefix;

	if (width <= 0 || height <= 0 || datasrc == NULL) return 0;
	if (width != dst->width || height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	prefixlen = (width > height ? width : height) + 1;
	hpass  = (int*)malloc((long int)width * height * sizeof(int));
	prefix = (int*)malloc(prefixlen * sizeof(int));
	if (!hpass || !prefix) { free(hpass); free(prefix); return 0; }

	for (y = 0; y < height; y++) {
		unsigned char* row = datasrc + y * bpl_src;
		prefix[0] = 0;
		for (x = 0; x < width; x++)
			prefix[x + 1] = prefix[x] + (row[x] == 255 ? 1 : 0);
		for (x = 0; x < width; x++) {
			lo = x - k; if (lo < 0) lo = 0;
			hi = x + k + 1; if (hi > width) hi = width;
			hpass[y * width + x] = (prefix[hi] - prefix[lo] > 0) ? 255 : 0;
		}
	}

	for (x = 0; x < width; x++) {
		prefix[0] = 0;
		for (y = 0; y < height; y++)
			prefix[y + 1] = prefix[y] + (hpass[y * width + x] == 255 ? 1 : 0);
		for (y = 0; y < height; y++) {
			lo = y - k; if (lo < 0) lo = 0;
			hi = y + k + 1; if (hi > height) hi = height;
			datadst[y * bpl_dst + x] = (unsigned char)((prefix[hi] - prefix[lo] > 0) ? 255 : 0);
		}
	}

	free(hpass);
	free(prefix);
	return 1;
}

int vc_binary_erode_fast(IVC* src, IVC* dst, int kernel)
{
	if (src == NULL || dst == NULL) return 0;
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bpl_src = src->bytesperline;
	int bpl_dst = dst->bytesperline;
	int k = kernel / 2;
	int x, y, lo, hi;
	int prefixlen;
	int* hpass;
	int* prefix;

	if (width <= 0 || height <= 0 || datasrc == NULL) return 0;
	if (width != dst->width || height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	prefixlen = (width > height ? width : height) + 1;
	hpass  = (int*)malloc((long int)width * height * sizeof(int));
	prefix = (int*)malloc(prefixlen * sizeof(int));
	if (!hpass || !prefix) { free(hpass); free(prefix); return 0; }

	for (y = 0; y < height; y++) {
		unsigned char* row = datasrc + y * bpl_src;
		prefix[0] = 0;
		for (x = 0; x < width; x++)
			prefix[x + 1] = prefix[x] + (row[x] == 255 ? 1 : 0);
		for (x = 0; x < width; x++) {
			lo = x - k; if (lo < 0) lo = 0;
			hi = x + k + 1; if (hi > width) hi = width;
			hpass[y * width + x] = (prefix[hi] - prefix[lo] == hi - lo) ? 255 : 0;
		}
	}

	for (x = 0; x < width; x++) {
		prefix[0] = 0;
		for (y = 0; y < height; y++)
			prefix[y + 1] = prefix[y] + (hpass[y * width + x] == 255 ? 1 : 0);
		for (y = 0; y < height; y++) {
			lo = y - k; if (lo < 0) lo = 0;
			hi = y + k + 1; if (hi > height) hi = height;
			datadst[y * bpl_dst + x] = (unsigned char)((prefix[hi] - prefix[lo] == hi - lo) ? 255 : 0);
		}
	}

	free(hpass);
	free(prefix);
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

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x;
			pos_mask = y * bytesperline_mask + x;
			pos_dst = y * bytesperline_dst + x;

			if (datamask[pos_mask] == 255)
				datadst[pos_dst] = datasrc[pos_src];
			else
				datadst[pos_dst] = 0; 
		}
	}

	return 1;
}


OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc;
	unsigned char* datadst;
	int width;
	int height;
	int bytesperline;
	int channels;
	int x, y, a, b;
	long int posX, posA, posB, posC, posD;
	int labeltable[16384] = { 0 };
	int label = 1;
	int num, tmplabel;
	OVC* blobs;
	int* label_map;
	int* new_label;

	if (src == NULL || dst == NULL || nlabels == NULL) return NULL;

	datasrc      = (unsigned char*)src->data;
	datadst      = (unsigned char*)dst->data;
	width        = src->width;
	height       = src->height;
	bytesperline = src->bytesperline;
	channels     = src->channels;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return NULL;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	label_map = (int*)calloc((long int)width * height, sizeof(int));
	if (!label_map) return NULL;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			label_map[y * width + x] = (datasrc[y * bytesperline + x * channels] != 0) ? 255 : 0;

	for (y = 0; y < height; y++) {
		label_map[y * width + 0] = 0;
		label_map[y * width + (width - 1)] = 0;
	}
	for (x = 0; x < width; x++) {
		label_map[0 * width + x] = 0;
		label_map[(height - 1) * width + x] = 0;
	}

	for (y = 1; y < height - 1; y++) {
		for (x = 1; x < width - 1; x++) {
			posA = (y - 1) * width + (x - 1);
			posB = (y - 1) * width + x;
			posC = (y - 1) * width + (x + 1);
			posD = y * width + (x - 1);
			posX = y * width + x;

			if (label_map[posX] != 0) {
				if (label >= 16383) { label_map[posX] = 0; continue; }

				if ((label_map[posA] == 0) && (label_map[posB] == 0) && (label_map[posC] == 0) && (label_map[posD] == 0)) {
					label_map[posX] = label;
					labeltable[label] = label;
					label++;
				} else {
					num = 16382;
					if (label_map[posA] != 0) num = labeltable[label_map[posA]];
					if ((label_map[posB] != 0) && (labeltable[label_map[posB]] < num)) num = labeltable[label_map[posB]];
					if ((label_map[posC] != 0) && (labeltable[label_map[posC]] < num)) num = labeltable[label_map[posC]];
					if ((label_map[posD] != 0) && (labeltable[label_map[posD]] < num)) num = labeltable[label_map[posD]];

					label_map[posX] = num;
					labeltable[num] = num;

					if (label_map[posA] != 0 && labeltable[label_map[posA]] != num) {
						for (tmplabel = labeltable[label_map[posA]], a = 1; a < label; a++)
							if (labeltable[a] == tmplabel) labeltable[a] = num;
					}
					if (label_map[posB] != 0 && labeltable[label_map[posB]] != num) {
						for (tmplabel = labeltable[label_map[posB]], a = 1; a < label; a++)
							if (labeltable[a] == tmplabel) labeltable[a] = num;
					}
					if (label_map[posC] != 0 && labeltable[label_map[posC]] != num) {
						for (tmplabel = labeltable[label_map[posC]], a = 1; a < label; a++)
							if (labeltable[a] == tmplabel) labeltable[a] = num;
					}
					if (label_map[posD] != 0 && labeltable[label_map[posD]] != num) {
						for (tmplabel = labeltable[label_map[posD]], a = 1; a < label; a++)
							if (labeltable[a] == tmplabel) labeltable[a] = num;
					}
				}
			}
		}
	}

	for (y = 1; y < height - 1; y++)
		for (x = 1; x < width - 1; x++) {
			posX = y * width + x;
			if (label_map[posX] != 0)
				label_map[posX] = labeltable[label_map[posX]];
		}

	for (a = 1; a < label - 1; a++)
		for (b = a + 1; b < label; b++)
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;

	*nlabels = 0;
	for (a = 1; a < label; a++) {
		if (labeltable[a] != 0) {
			labeltable[*nlabels] = labeltable[a];
			(*nlabels)++;
		}
	}

	if (*nlabels == 0) { free(label_map); return NULL; }

	new_label = (int*)calloc(label, sizeof(int));
	if (!new_label) { free(label_map); return NULL; }
	for (a = 0; a < *nlabels; a++)
		new_label[labeltable[a]] = a + 1;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++) {
			int lbl = label_map[y * width + x];
			datadst[y * bytesperline + x * channels] =
				(lbl > 0 && lbl < label && new_label[lbl] > 0)
				? (unsigned char)new_label[lbl] : 0;
		}

	blobs = (OVC*)calloc(*nlabels, sizeof(OVC));
	if (!blobs) { free(new_label); free(label_map); return NULL; }
	for (a = 0; a < *nlabels; a++) blobs[a].label = a + 1;

	free(new_label);
	free(label_map);
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

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;
		blobs[i].perimeter = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					blobs[i].area++;

					sumx += x;
					sumy += y;

					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					if ((data[pos - 1] != blobs[i].label) ||
						(data[pos + 1] != blobs[i].label) ||
						(data[pos - bytesperline] != blobs[i].label) ||
						(data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

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

		int size = 5;

		for (t = -thickness / 2; t <= thickness / 2; t++)
		{
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

static int compare_uchar(const void* a, const void* b)
{
	return (*(unsigned char*)a - *(unsigned char*)b);
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


int compare_int(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

OVC* vc_blob_filter_deformation(OVC* blobs, int* nblobs)
{
	int i, count = 0;

	if (blobs == NULL || nblobs == NULL || *nblobs <= 0)
		return NULL;

	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].deformation <= VC_FILTER_DEFORM_MAX &&
			blobs[i].diameter_mm >= VC_MIN_DIAMETER_MM)
			count++;
	}

	if (count == 0)
	{
		*nblobs = 0;
		return NULL;
	}

	OVC* filtered = (OVC*)calloc(count, sizeof(OVC));
	if (filtered == NULL) return NULL;

	int j = 0;
	for (i = 0; i < *nblobs; i++)
	{
		if (blobs[i].deformation <= VC_FILTER_DEFORM_MAX &&
			blobs[i].diameter_mm >= VC_MIN_DIAMETER_MM)
			filtered[j++] = blobs[i];
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

int vc_orange_classify(OVC* blobs, int nblobs)
{
	int i, c;
	const float px_to_mm = VC_PX_TO_MM;
	const float pi = 3.14159265f;

	if (blobs == NULL || nblobs <= 0) return 0;

	for (i = 0; i < nblobs; i++)
	{
		float diam_px = (blobs[i].width + blobs[i].height) / 2.0f;
		blobs[i].diameter_mm = diam_px * px_to_mm;

		
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

		float max_dim = (blobs[i].width > blobs[i].height)
			? (float)blobs[i].width : (float)blobs[i].height;
		float ideal_area = pi * (max_dim / 2.0f) * (max_dim / 2.0f);
		float diff = (float)blobs[i].area - ideal_area;
		if (diff < 0.0f) diff = -diff;
		blobs[i].deformation = (ideal_area > 0.0f)
			? (diff / ideal_area * 100.0f) : 0.0f;

		if (blobs[i].deformation < VC_DEFORM_EXTRA)
			blobs[i].category = 0; 
		else if (blobs[i].deformation < VC_DEFORM_I)
			blobs[i].category = 1; 
		else if (blobs[i].deformation < VC_DEFORM_II)
			blobs[i].category = 2; 
		else
			blobs[i].category = 3; 

		{
			int spot_cat;
			if      (blobs[i].spot_ratio < VC_SPOT_EXTRA) spot_cat = 0;
			else if (blobs[i].spot_ratio < VC_SPOT_I)     spot_cat = 1;
			else if (blobs[i].spot_ratio < VC_SPOT_II)    spot_cat = 2;
			else                                            spot_cat = 3;

			if (spot_cat > blobs[i].category) blobs[i].category = spot_cat;
		}
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


int vc_count_new_oranges(OVC* blobs, int nlabels, int* prev_xc, int* prev_yc, int* prev_count, int* total)
{
	int i, j, nova;
	int dx, dy;

	if (blobs == NULL || prev_xc == NULL || prev_yc == NULL || prev_count == NULL || total == NULL)
		return 0;

	for (i = 0; i < nlabels; i++)
	{
		nova = 1; 

		for (j = 0; j < *prev_count; j++)
		{
			dx = blobs[i].xc - prev_xc[j];
			dy = blobs[i].yc - prev_yc[j];

			
			if ((dx * dx + dy * dy) < (VC_DIST_MAX * VC_DIST_MAX))
			{
				nova = 0;
				break;
			}
		}

		if (nova)
			(*total)++;
	}

	*prev_count = nlabels;
	if (nlabels > VC_MAX_BLOBS) nlabels = VC_MAX_BLOBS;
	for (i = 0; i < nlabels; i++)
	{
		prev_xc[i] = blobs[i].xc;
		prev_yc[i] = blobs[i].yc;
	}

	return 1;
}

int vc_blob_count_dark_spots(IVC* spot_binary, IVC* label_image, OVC* blobs, int nblobs)
{
	unsigned char* spot;
	unsigned char* labels;
	int width, height, bpl_spot, bpl_labels;
	int i, x, y;
	long int pos_spot, pos_label;
	int idx[256];

	if (spot_binary == NULL || label_image == NULL || blobs == NULL || nblobs <= 0) return 0;
	if (spot_binary->channels != 1 || label_image->channels != 1) return 0;
	if (spot_binary->width != label_image->width || spot_binary->height != label_image->height) return 0;

	spot       = (unsigned char*)spot_binary->data;
	labels     = (unsigned char*)label_image->data;
	width      = spot_binary->width;
	height     = spot_binary->height;
	bpl_spot   = spot_binary->bytesperline;
	bpl_labels = label_image->bytesperline;

	for (i = 0; i < nblobs; i++) {
		blobs[i].dark_spot_area = 0;
		blobs[i].spot_ratio     = 0.0f;
	}

	memset(idx, -1, sizeof(idx));
	for (i = 0; i < nblobs; i++)
		if (blobs[i].label >= 1 && blobs[i].label <= 255)
			idx[blobs[i].label] = i;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos_spot  = (long int)y * bpl_spot   + x;
			pos_label = (long int)y * bpl_labels + x;
			if (spot[pos_spot] == 255) {
				int lbl = labels[pos_label];
				if (lbl > 0 && idx[lbl] >= 0)
					blobs[idx[lbl]].dark_spot_area++;
			}
		}
	}

	for (i = 0; i < nblobs; i++) {
		int total = blobs[i].area + blobs[i].dark_spot_area;
		blobs[i].spot_ratio = (total > 0)
			? (blobs[i].dark_spot_area * 100.0f / (float)total)
			: 0.0f;
	}
	return 1;
}



int vc_gray_local_mean_detect(IVC* masked_gray, IVC* dst, int kernel, int threshold)
{
	unsigned char* src = masked_gray->data;
	unsigned char* out = dst->data;

	int width = masked_gray->width;
	int height = masked_gray->height;
	int bpl = masked_gray->bytesperline;

	if (!masked_gray || !dst) return 0;
	if (!src || !out) return 0;
	if (width <= 0 || height <= 0) return 0;
	if (width != dst->width || height != dst->height) return 0;
	if (masked_gray->channels != 1 || dst->channels != 1) return 0;

	int k = kernel / 2;

	int* isum = calloc(width * height, sizeof(int));
	int* icnt = calloc(width * height, sizeof(int));

	if (!isum || !icnt) {
		free(isum);
		free(icnt);
		return 0;
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int idx = y * width + x;

			int v = src[y * bpl + x];

			int val = (v > 0) ? v : 0;
			int cnt = (v > 0) ? 1 : 0;

			int left = (x > 0) ? idx - 1 : -1;
			int above = (y > 0) ? idx - width : -1;
			int diag = (x > 0 && y > 0) ? idx - width - 1 : -1;

			isum[idx] =
				val
				+ (left >= 0 ? isum[left] : 0)
				+ (above >= 0 ? isum[above] : 0)
				- (diag >= 0 ? isum[diag] : 0);

			icnt[idx] =
				cnt
				+ (left >= 0 ? icnt[left] : 0)
				+ (above >= 0 ? icnt[above] : 0)
				- (diag >= 0 ? icnt[diag] : 0);
		}
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int pos = y * bpl + x;

			if (src[pos] == 0) {
				out[pos] = 0;
				continue;
			}

			int x0 = (x - k > 0) ? x - k : 0;
			int y0 = (y - k > 0) ? y - k : 0;
			int x1 = (x + k < width - 1) ? x + k : width - 1;
			int y1 = (y + k < height - 1) ? y + k : height - 1;

			int A = isum[y1 * width + x1];
			int B = (x0 > 0) ? isum[y1 * width + (x0 - 1)] : 0;
			int C = (y0 > 0) ? isum[(y0 - 1) * width + x1] : 0;
			int D = (x0 > 0 && y0 > 0)
				? isum[(y0 - 1) * width + (x0 - 1)] : 0;

			int window_sum = A - B - C + D;

			int NA = icnt[y1 * width + x1];
			int NB = (x0 > 0) ? icnt[y1 * width + (x0 - 1)] : 0;
			int NC = (y0 > 0) ? icnt[(y0 - 1) * width + x1] : 0;
			int ND = (x0 > 0 && y0 > 0)
				? icnt[(y0 - 1) * width + (x0 - 1)] : 0;

			int window_count = NA - NB - NC + ND;

			if (window_count == 0) {
				out[pos] = 0;
				continue;
			}

			int mean = window_sum / window_count;

			int residual = mean - (int)src[pos];

			out[pos] = (residual > threshold) ? 255 : 0;
		}
	}

	free(isum);
	free(icnt);

	return 1;
}

typedef struct {
	int sum;
	int cnt;
} Cell;

int vc_gray_local_mean_detect_fast(IVC* img, IVC* dst, int kernel, int threshold)
{
	unsigned char* src = img->data;
	unsigned char* out = dst->data;

	int w = img->width;
	int h = img->height;
	int bpl = img->bytesperline;

	int k = kernel / 2;

	Cell* sat = calloc(w * h, sizeof(Cell));
	if (!sat) return 0;

	for (int y = 0; y < h; y++) {
		Cell* row = sat + y * w;
		Cell* row_up = (y > 0) ? sat + (y - 1) * w : NULL;

		for (int x = 0; x < w; x++) {

			int v = src[y * bpl + x];

			int val = (v > 0) ? v : 0;
			int cnt = (v > 0) ? 1 : 0;

			Cell left = (x > 0) ? row[x - 1] : (Cell) { 0, 0 };
			Cell above = (y > 0) ? row_up[x] : (Cell) { 0, 0 };
			Cell diag = (x > 0 && y > 0) ? row_up[x - 1] : (Cell) { 0, 0 };

			row[x].sum = val + left.sum + above.sum - diag.sum;
			row[x].cnt = cnt + left.cnt + above.cnt - diag.cnt;
		}
	}

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {

			int pos = y * bpl + x;
			int v = src[pos];

			if (!v) {
				out[pos] = 0;
				continue;
			}

			int x0 = (x - k > 0) ? x - k : 0;
			int y0 = (y - k > 0) ? y - k : 0;
			int x1 = (x + k < w - 1) ? x + k : w - 1;
			int y1 = (y + k < h - 1) ? y + k : h - 1;

			Cell A = sat[y1 * w + x1];
			Cell B = (x0 > 0) ? sat[y1 * w + x0 - 1] : (Cell) { 0, 0 };
			Cell C = (y0 > 0) ? sat[(y0 - 1) * w + x1] : (Cell) { 0, 0 };
			Cell D = (x0 > 0 && y0 > 0) ? sat[(y0 - 1) * w + x0 - 1] : (Cell) { 0, 0 };

			int sum = A.sum - B.sum - C.sum + D.sum;
			int cnt = A.cnt - B.cnt - C.cnt + D.cnt;

			if (!cnt) {
				out[pos] = 0;
				continue;
			}

			int mean = sum / cnt;
			int residual = mean - v;

			out[pos] = (residual > threshold) ? 255 : 0;
		}
	}

	free(sat);
	return 1;
}

int vc_binary_subtract(IVC* src, IVC* subtrahend, IVC* dst)
{
	unsigned char* datasrc;
	unsigned char* datasub;
	unsigned char* datadst;
	int width, height, bpl;
	int x, y;
	long int pos;

	if (src == NULL || subtrahend == NULL || dst == NULL) return 0;

	datasrc = src->data;
	datasub = subtrahend->data;
	datadst = dst->data;
	width   = src->width;
	height  = src->height;
	bpl     = src->bytesperline;

	if (datasrc == NULL || datasub == NULL || datadst == NULL) return 0;
	if (width <= 0 || height <= 0) return 0;
	if (width != subtrahend->width || height != subtrahend->height) return 0;
	if (width != dst->width       || height != dst->height)        return 0;
	if (src->channels != 1 || subtrahend->channels != 1 || dst->channels != 1) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = (long int)y * bpl + x;
			datadst[pos] = (datasrc[pos] == 255 && datasub[pos] == 255) ? 0 : datasrc[pos];
		}
	}
	return 1;
}