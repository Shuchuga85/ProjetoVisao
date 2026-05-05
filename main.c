#include <stdio.h>
#include "Windows.h"
#include "stdlib.h"
#include "vc.h"
#include "pc.h"

/// <summary>
/// Ex Criar Imagen Binaria
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main1(void)
{
    IVC* image;
    int x, y;
    long int pos;

    image = vc_image_new(320, 280, 1, 1);
    if (image == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    for (x = 0; x < image->width; x++)
    {
        for (y = 0; y < image->height; y++)
        {
            pos = y * image->bytesperline + x * image->channels;

            if ((x < image->width / 2) && (y < image->height / 2))
                image->data[pos] = 1;
            else if ((x > image->width / 2) && (y > image->height / 2))
                image->data[pos] = 1;
            else
                image->data[pos] = 0;
        }
    }

    vc_write_image("teste.pbm", image);

    vc_image_free(image);~

    system("FilterGear teste.pbm");

    printf("Press any key to exit...\n");
    getchar();


    return 0;
}


/// <summary>
/// Ex Gradiente horizontal
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main2(void)
{
    IVC* image;
    int x, y;
    long int pos;

    image = vc_image_new(256, 256, 1, 255);
    if (image == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    for (y = 0; y < image->height; y++)
    {
        for (x = 0; x < image->width; x++)
        {
            pos = y * image->bytesperline + x * image->channels;
            image->data[pos] = (unsigned char)x;
        }
    }

    vc_write_image("gradiente.pgm", image);

    vc_image_free(image);

    system("FilterGear gradiente.pgm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}

/// <summary>
/// Ex Gray Negative
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main3(void)
{
    IVC* image;
    int x, y;
    long int pos;

    image = vc_read_image("src/lighthouse.pgm");
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    vc_gray_negative(image);

    vc_write_image("result.pgm", image);

    vc_image_free(image);

    // Abrir imagem no FilterGear
    system("FilterGear result.pgm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex RGB negative
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main4(void)
{
    IVC* image;
    int x, y;
    long int pos;

    image = vc_read_image("src/baboon.ppm");
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    vc_rgb_negative(image);

    vc_write_image("result.ppm", image);

    vc_image_free(image);

    system("FilterGear result.ppm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex RGB to Gray
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main5(void)
{
    IVC* image;
    IVC* result;

    image = vc_read_image("src/baboon.ppm");
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, image->levels);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    vc_rgb_to_gray(image, result);

    vc_write_image("result.pgm", result);

    vc_image_free(image);
    vc_image_free(result);

    system("FilterGear result.pgm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex RGB to HSV and HSV segmentation
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main6(void)
{
    IVC* image;
    IVC* result;
    IVC* result2;
    int x, y;
    long int pos;

    image = vc_read_image("src/hsvtest.ppm");
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, image->channels, image->levels);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result2 = vc_image_new(image->width, image->height, 1, image->levels);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    vc_rgb_to_hsv(image, result);

    vc_write_image("result.ppm", result);

    vc_hsv_segmentation(result, result2 ,340, 20, 50, 100, 60, 100);

    vc_write_image("result2.ppm", result2);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", "result.ppm");
    run_process("FilterGear.exe", "result2.ppm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex Detectao de atividade cerebral
/// </summary>
/// <param name=""></param>
/// <returns></returns>

int maiddn(void)
{
    IVC* image;
    IVC* hsv;
    IVC* mask;
    IVC* brain_mask;

    int total_brain_pixels;
    int count;
    float percent;

    char* filenames[2] = { "src/PET-Normal.ppm", "src/PET-Alzheimer.ppm" };
    int i;

    for (i = 0; i < 2; i++)
    {
        printf("\n--- Analise da imagem: %s ---\n", filenames[i]);

        image = vc_read_image(filenames[i]);
        if (image == NULL) return 0;

        hsv = vc_image_new(image->width, image->height, 3, 255);
        mask = vc_image_new(image->width, image->height, 1, 255);
        brain_mask = vc_image_new(image->width, image->height, 1, 255);

        //vc_rgb_to_gray(image,brain_mask);
        
        vc_rgb_to_hsv(image, hsv);

        vc_hsv_segmentation(hsv, brain_mask, 0, 360, 0, 50, 0, 30);

        vc_gray_negative(brain_mask);

        total_brain_pixels = vc_count_white_pixels(brain_mask);

        printf("Total brain pixels: %d\n", total_brain_pixels);

        /*
        vc_write_image("mask.ppm", brain_mask);
        PROCESS_INFORMATION p1 = run_process("FilterGear.exe", "mask.ppm");
        getchar();
        kill_process(p1);
        */
      
        
        vc_hsv_segmentation(hsv, mask, 161, 290, 50, 100, 50, 100);
        count = vc_count_white_pixels(mask);
        percent = (float)((float)count / (float)total_brain_pixels) * 100.0f;
        printf("Total pixels: %d\n", count);
        printf("0-25%% atividade: %.2f%%\n", percent);

        vc_hsv_segmentation(hsv, mask, 71, 160, 50, 100, 60, 100);
        count = vc_count_white_pixels(mask);
        percent = (float)((float)count / (float)total_brain_pixels) * 100.0;
        printf("Total pixels: %d\n", count);
        printf("26-50%% atividade: %.2f%%\n", percent);

        vc_hsv_segmentation(hsv, mask, 46, 70, 50, 100, 50, 100);
        count = vc_count_white_pixels(mask);
        percent = (float)((float)count / (float)total_brain_pixels) * 100.0;
        printf("Total pixels: %d\n", count);
        printf("51-75%% atividade: %.2f%%\n", percent);

        vc_hsv_segmentation(hsv, mask, 291,360, 60, 100, 60, 100);
        count = vc_count_white_pixels(mask);
        vc_hsv_segmentation(hsv, mask, 0, 45, 60, 100, 60, 100);
        count += vc_count_white_pixels(mask);
        percent = (float)((float)count / (float)total_brain_pixels) * 100.0;
        printf("Total pixels: %d\n", count);
        printf("76-100%% atividade: %.2f%%\n", percent);

        vc_image_free(image);
        vc_image_free(hsv);
        vc_image_free(mask);
        vc_image_free(brain_mask);
    }

    printf("\nPressione uma tecla para sair...\n");
    getchar();
    return 0;
}

int mai33n(void)
{
    IVC* image;
    IVC* hsv;

    char* filenames[2] = { "src/PET-Normal.ppm", "src/PET-Alzheimer.ppm" };
    int i;

    for (i = 0; i < 2; i++)
    {
        printf("\n--- Analise da imagem: %s ---\n", filenames[i]);

        image = vc_read_image(filenames[i]);
        if (image == NULL)
        {
            printf("Erro ao ler imagem.\n");
            getchar();
            return 0;
        }

        hsv = vc_image_new(image->width, image->height, 3, 255);

        vc_rgb_to_hsv(image, hsv);

        //vc_pet_activity_analysis(hsv);

        vc_image_free(image);
        vc_image_free(hsv);
    }

    printf("\nPressione uma tecla para sair...\n");
    getchar();
    return 0;
}

/// <summary>
/// EX GrayScale image to RGB
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int main9(void)
{
    IVC* image;
    IVC* result;
    char* filename = "src/flir-02.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 3, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_scale_gray_to_color_palette(image, result);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}

/// <summary>
/// EX GrayScale Image to Binary in threshold
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mai1n(void)
{
    IVC* image;
    IVC* result;
    char* filename = "src/cells.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }



    vc_gray_to_binary(image, result, 120);
    vc_gray_negative(result);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex GrayScale image to binary in avarage
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mainddd(void)
{
    IVC* image;
    IVC* result;
    char* filename = "src/coins.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary_global_mean(image, result);
    vc_gray_negative(result);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}

/// <summary>
/// Ex GrayScale image to binary in midpoint
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mainfs(void)
{
    IVC* image;
    IVC* result;
    char* filename = "src/cells.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary_midpoint(image, result, 25);
    vc_gray_negative(result);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}

/// <summary>
/// Ex GrayScale image to binary in bernsen
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mainasd(void)
{
    IVC* image;
    IVC* result;
    char* filename = "src/cells.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary_bernsen(image, result, 25, 15);
    vc_gray_negative(result);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex GrayScale image to binary in niblack
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mafden(void)
{
    IVC* image;
    IVC* result;
    char* filename = "src/cells.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary_niblack(image, result, 25, -0.2f);
    vc_gray_negative(result);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}



/// <summary>
/// Ex dilatation by erode
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mfrain(void)
{
    IVC* image;
    IVC* result;
    IVC* temp;
    char* filename = "src/flir-01.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    temp = vc_image_new(image->width, image->height, 1, 255);
    if (temp == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary(image, temp, 127);
    vc_binary_erode(temp, result, 3);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex dilatation by dilate
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mainooo(void)
{
    IVC* image;
    IVC* result;
    IVC* temp;
    char* filename = "src/flir-01.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    temp = vc_image_new(image->width, image->height, 1, 255);
    if (temp == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary(image, temp, 127);
    vc_binary_dilate(temp, result, 3);

    vc_write_image("temp.ppm", temp);
    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(temp);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result2.ppm");
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex dilatation by open
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int maifdfn(void)
{
    IVC* image;
    IVC* result;
    IVC* temp;
    char* filename = "src/flir-01.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    temp = vc_image_new(image->width, image->height, 1, 255);
    if (temp == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary(image, temp, 127);
    vc_binary_open(temp, result, 3, 5);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}




/// <summary>
/// Ex dilatation by close
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mainsad(void)
{
    IVC* image;
    IVC* result;
    IVC* temp;
    char* filename = "src/flir-01.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    temp = vc_image_new(image->width, image->height, 1, 255);
    if (temp == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    vc_gray_to_binary(image, temp, 127);
    vc_binary_close(temp, result, 5, 3);

    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}




/// <summary>
/// Ex2 crop brain from cranium image
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int mdfain(void)
{
    IVC* image;
    IVC* result;

    enum { templength = 4 };
    IVC* temps[templength];

    char* filename = "src/brain.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    for (int i = 0; i < templength; i++)
    {
        temps[i] = vc_image_new(image->width, image->height, 1, 255);
        if (temps[i] == NULL)
        {
            printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
            getchar();
            return 0;
        }
    }

    
  
    vc_gray_to_binary_range(image, temps[0], 80, 190);
    vc_binary_erode(temps[0], temps[1], 8);
    vc_binary_dilate(temps[1], temps[2], 5);
    vc_binary_close(temps[2], temps[3], 10, 1);
    vc_mask_crop(image, temps[3], result);


    vc_write_image("result.ppm",  result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex Entiquetagem e blobs
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int madfin(void)
{
    IVC* image;
    IVC* result;

    char* filename = "src/labelling-2.pgm";

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }


    int nlabels;
    OVC* blob = vc_binary_blob_labelling(image, result, &nlabels);
    vc_binary_blob_info(result, blob, nlabels);

    printf("Number of blobs: %d\n", nlabels);
    
    vc_write_image("result.ppm", result);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex2 
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int maidfdn(void)
{

    char* filename = "src/input.ppm";

    IVC* image;
    IVC* result;
    IVC* hsv;
    enum { templength = 4 };
    IVC* temps[templength];
    


    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    hsv = vc_image_new(image->width, image->height, 3, 255);
    if (hsv == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }
    

    for (int i = 0; i < templength; i++)
    {
        temps[i] = vc_image_new(image->width, image->height, 1, 255);
        if (temps[i] == NULL)
        {
            printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
            getchar();
            return 0;
        }
    }
    
    vc_rgb_to_hsv(image, hsv);
    vc_hsv_segmentation(hsv, temps[1], 235, 247, 55, 100, 55, 100);
    vc_binary_close(temps[1], temps[3], 3, 3);

    int nlabels;
    OVC* blobs = vc_binary_blob_labelling(temps[3], temps[2], &nlabels);
    vc_binary_blob_info(temps[2], blobs, nlabels);
    vc_draw_blobs(temps[3], blobs, nlabels, 255, 255, 255, 255, 255, 255, 2);


    vc_write_image("result.ppm", temps[1]);

    vc_image_free(image);
    vc_image_free(result);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");


    printf("Press any key to exit...\n");
    getchar();

    return 0;
}



/// <summary>
/// Ex histogram equalization
/// </summary>
/// <param name=""></param>
/// <returns></returns>
int madffgin(void)
{

    char* filename = "src/barbara_heq.pgm";

    IVC* image;
    IVC* result;
    IVC* gray;
    IVC* histogram;

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    result = vc_image_new(image->width, image->height, 1, 255);
    if (result == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    gray = vc_image_new(image->width, image->height, 1, 255);
    if (gray == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    histogram = vc_image_new(256, 256, 1, 255);
    if (histogram == NULL)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    if (image->channels == 3)
    {
        vc_rgb_to_gray(image, gray);
    }
    else
    {
        gray = image;
    }

    vc_gray_histogram_equalization(gray, result);
    vc_gray_histogram_show(gray, histogram);

    vc_write_image("result.pgm", result);
    vc_write_image("result2.pgm", histogram);

    vc_image_free(image);
    vc_image_free(result);
    //vc_image_free(gray);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.pgm");
    run_process("FilterGear.exe", "result2.pgm");



    printf("Press any key to exit...\n");
    getchar();

    return 0;
}

/// <summary>
/// Ex gray edge gradients
/// </summary>
int mainEdge(void)
{
    char* filename = "src/barbara.pgm";

    IVC* image;
    IVC* gray;
    IVC* prewitt;
    IVC* sobel;

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    gray = vc_image_new(image->width, image->height, 1, 255);
    prewitt = vc_image_new(image->width, image->height, 1, 255);
    sobel = vc_image_new(image->width, image->height, 1, 255);

    if ((gray == NULL) || (prewitt == NULL) || (sobel == NULL))
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    if (image->channels == 3)
        vc_rgb_to_gray(image, gray);
    else
        gray = image;

    vc_gray_edge_prewitt(gray, prewitt, 0.8f);
    vc_gray_edge_sobel(gray, sobel, 0.8f);

    vc_write_image("prewitt.pgm", prewitt);
    vc_write_image("sobel.pgm", sobel);

    vc_image_free(image);
    vc_image_free(prewitt);
    vc_image_free(sobel);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "prewitt.pgm");
    run_process("FilterGear.exe", "sobel.pgm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Ex low-pass filters: mean, median, gaussian
/// </summary>
int mainLowPass(void)
{
    char* filename = "src/VC-10-barbara-sp.pgm";

    IVC* image;
    IVC* mean;
    IVC* median;
    IVC* gaussian;

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    if (image->channels != 1)
    {
        printf("ERROR: expected grayscale image.\n");
        vc_image_free(image);
        getchar();
        return 0;
    }

    mean    = vc_image_new(image->width, image->height, 1, 255);
    median  = vc_image_new(image->width, image->height, 1, 255);
    gaussian = vc_image_new(image->width, image->height, 1, 255);

    if (!mean || !median || !gaussian)
    {
        printf("ERROR -> vc_image_new():\n\tOut of memory!\n");
        getchar();
        return 0;
    }

    vc_gray_lowpass_mean_filter(image, mean, 5);
    vc_gray_lowpass_median_filter(image, median, 5);
    vc_gray_lowpass_gaussian_filter(image, gaussian);

    vc_write_image("mean.pgm",    mean);
    vc_write_image("median.pgm",  median);
    vc_write_image("gaussian.pgm", gaussian);

    printf("Results written:\n");
    printf("  mean.pgm\n");
    printf("  median.pgm\n");
    printf("  gaussian.pgm\n");

    vc_image_free(image);
    vc_image_free(mean);
    vc_image_free(median);
    vc_image_free(gaussian);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "mean.pgm");
    run_process("FilterGear.exe", "median.pgm");
    run_process("FilterGear.exe", "gaussian.pgm");

    printf("Press any key to exit...\n");
    getchar();

    return 0;
}


/// <summary>
/// Orange detection and EEC 379/71 classification on conveyor belt image
/// </summary>
int main(void)
{
    char* filename = "src/input.ppm";

    IVC* image = NULL;
    IVC* hsv = NULL;
    IVC* mask = NULL;
    IVC* temp = NULL;
    IVC* labeled = NULL;
    OVC* blobs = NULL;
    OVC* filtered = NULL;
    int nlabels = 0;
    int i;

    image = vc_read_image(filename);
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image(): could not open %s\n", filename);
        getchar();
        return 1;
    }

    hsv     = vc_image_new(image->width, image->height, 3, 255);
    mask    = vc_image_new(image->width, image->height, 1, 255);
    temp    = vc_image_new(image->width, image->height, 1, 255);
    labeled = vc_image_new(image->width, image->height, 1, 255);

    if (!hsv || !mask || !temp || !labeled)
    {
        printf("ERROR -> vc_image_new(): out of memory\n");
        goto done;
    }

    // Segment orange hue: H 10-40deg, S 45-100%, V 40-100%
    vc_rgb_to_hsv(image, hsv);
    vc_hsv_segmentation(hsv, mask, 10, 40, 45, 100, 40, 100);

    // Close small holes in the segmented mask
    vc_binary_close(mask, temp, 5, 3);

    // Label connected regions
    blobs = vc_binary_blob_labelling(temp, labeled, &nlabels);
    if (blobs == NULL || nlabels == 0)
    {
        printf("No blobs detected.\n");
        goto done;
    }
    vc_binary_blob_info(labeled, blobs, nlabels);

    // Remove noise and non-orange fruits (area threshold)
    filtered = vc_blob_filter_area(blobs, &nlabels, 2000);
    free(blobs);
    blobs = filtered;
    filtered = NULL;

    if (blobs == NULL || nlabels == 0)
    {
        printf("No oranges remaining after area filter.\n");
        goto done;
    }

    // Compute calibre, deformation and EEC category per blob
    vc_orange_classify(blobs, nlabels);

    // Draw green bounding boxes and red centroids on the original RGB image
    vc_draw_blobs(image, blobs, nlabels, 0, 255, 0, 255, 0, 0, 2);

    // Print classification results to stdout
    printf("Laranjas detectadas: %d\n\n", nlabels);
    for (i = 0; i < nlabels; i++)
    {
        printf("LABEL:%d  AREA:%d  PERIMETRO:%d\n",
               blobs[i].label, blobs[i].area, blobs[i].perimeter);
        printf("  CALIBRE:%d  DIAMETRO:%.1fMM\n",
               blobs[i].calibre, blobs[i].diameter_mm);
        printf("  DEFORMACAO:%.2f%%  CATEGORIA:%s\n\n",
               blobs[i].deformation, vc_orange_category_name(blobs[i].category));
    }

    vc_write_image("result.ppm", image);
    vc_write_image("mask.pgm", temp);

done:
    free(blobs);
    vc_image_free(labeled);
    vc_image_free(temp);
    vc_image_free(mask);
    vc_image_free(hsv);
    vc_image_free(image);

    run_process("FilterGear.exe", filename);
    run_process("FilterGear.exe", "result.ppm");

    printf("Press any key to exit...\n");
    getchar();
    return 0;
}