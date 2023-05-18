#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define LAPLACIAN_THREADS 4 // change the number of threads as you run your concurrency experiment

/* Laplacian filter is 3 by 3 */
#define FILTER_WIDTH 3
#define FILTER_HEIGHT 3

#define RGB_COMPONENT_COLOR 255

typedef struct
{
    unsigned char r, g, b;
} PPMPixel;

struct parameter
{
    PPMPixel *image;         // original image pixel data
    PPMPixel *result;        // filtered image pixel data
    unsigned long int w;     // width of image
    unsigned long int h;     // height of image
    unsigned long int start; // starting point of work
    unsigned long int size;  // equal share of work (almost equal if odd)
};

struct file_name_args
{
    char *input_file_name;     // e.g., file1.ppm
    char output_file_name[20]; // will take the form laplaciani.ppm, e.g., laplacian1.ppm
};

/*The total_elapsed_time is the total time taken by all threads
to compute the edge detection of all input images .
*/
double total_elapsed_time = 0;

/*This is the thread function. It will compute the new values for the region of image specified in params (start to start+size) using convolution.
    For each pixel in the input image, the filter is conceptually placed on top ofthe image with its origin lying on that pixel.
    The  values  of  each  input  image  pixel  under  the  mask  are  multiplied  by the corresponding filter values.
    Truncate values smaller than zero to zero and larger than 255 to 255.
    The results are summed together to yield a single output value that is placed in the output image at the location of the pixel being processed on the input.

 */
void *compute_laplacian_threadfn(void *params)
{
    struct parameter *p = (struct parameter *)params;

    int laplacian[FILTER_WIDTH][FILTER_HEIGHT] =
        {
            {-1, -1, -1},
            {-1, 8, -1},
            {-1, -1, -1}};

    int imageWidth = p->w;
    int imageHeight = p->h;
    PPMPixel *image = p->image;
    PPMPixel *result = p->result;
    unsigned long start = p->start;
    unsigned long size = p->size;

    unsigned long stop = start + size;

    for (unsigned long i = start; i < stop; i++)
    {
        for (unsigned long j = 0; j < imageWidth; j++)
        {
            int red = 0, green = 0, blue = 0;

            for (int k = 0; k < FILTER_HEIGHT; k++)
            {
                for (int l = 0; l < FILTER_WIDTH; l++)
                {
                    int x_coordinate = (j - FILTER_WIDTH / 2 + l + imageWidth) % imageWidth;
                    int y_coordinate = (i - FILTER_HEIGHT / 2 + k + imageHeight) % imageHeight;

                    red += image[y_coordinate * imageWidth + x_coordinate].r * laplacian[k][l];
                    green += image[y_coordinate * imageWidth + x_coordinate].g * laplacian[k][l];
                    blue += image[y_coordinate * imageWidth + x_coordinate].b * laplacian[k][l];
                }
            }

            red = (red < 0) ? 0 : (red > 255) ? 255
                                              : red;
            green = (green < 0) ? 0 : (green > 255) ? 255
                                                    : green;
            blue = (blue < 0) ? 0 : (blue > 255) ? 255
                                                 : blue;

            result[i * imageWidth + j].r = red;
            result[i * imageWidth + j].g = green;
            result[i * imageWidth + j].b = blue;
        }
    }

    return NULL;
}

/* Apply the Laplacian filter to an image using threads.
 Each thread shall do an equal share of the work, i.e. work=height/number of threads. If the size is not even, the last thread shall take the rest of the work.
 Compute the elapsed time and store it in *elapsedTime (Read about gettimeofday).
 Return: result (filtered image)
 */
PPMPixel *apply_filters(PPMPixel *image, unsigned long w, unsigned long h, double *elapsedTime)
{
    struct timeval start, end;

    // allocate memory for the result image
    PPMPixel *result = (PPMPixel *)malloc(sizeof(PPMPixel) * w * h);
    if (!result)
    {
        printf("Error: Unable to allocate memory for the result image.\n");
        exit(1);
    }

    // define parameters for the threads
    struct parameter *params = malloc(sizeof(struct parameter) * LAPLACIAN_THREADS);
    if (!params)
    {
        printf("Error: Unable to allocate memory for thread parameters.\n");
        free(result);
        exit(1);
    }

    pthread_t threads[LAPLACIAN_THREADS];
    unsigned long work = h / LAPLACIAN_THREADS;

    // get the start time
    gettimeofday(&start, NULL);

    // create threads
    for (int i = 0; i < LAPLACIAN_THREADS; i++)
    {
        params[i].image = image;
        params[i].result = result;
        params[i].w = w;
        params[i].h = h;
        params[i].start = i * work;
        params[i].size = (i == LAPLACIAN_THREADS - 1) ? (h - params[i].start) : work;

        if (pthread_create(&threads[i], NULL, compute_laplacian_threadfn, (void *)&params[i]) != 0)
        {
            printf("Error: Unable to create thread.\n");
            free(result);
            free(params);
            exit(1);
        }
    }

    // join threads
    for (int i = 0; i < LAPLACIAN_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // get the end time
    gettimeofday(&end, NULL);

    // calculate elapsed time in seconds
    *elapsedTime = (double)(end.tv_sec - start.tv_sec) +
                   (double)(end.tv_usec - start.tv_usec) / 1000000;

    // free parameters
    free(params);

    return result;
}

/*Create a new P6 file to save the filtered image in. Write the header block
 e.g. P6
      Width Height
      Max color value
 then write the image data.
 The name of the new file shall be "filename" (the second argument).
 */
void write_image(PPMPixel *image, char *filename, unsigned long int width, unsigned long int height)
{
    FILE *fp = fopen(filename, "wb");

    if (!fp)
    {
        printf("Error: Unable to open file for writing.\n");
        exit(1);
    }

    fprintf(fp, "P6\n%lu %lu\n255\n", width, height);
    fwrite(image, sizeof(PPMPixel), width * height, fp);
    fclose(fp);
}

/* Open the filename image for reading, and parse it.
    Example of a ppm header:    //http://netpbm.sourceforge.net/doc/ppm.html
    P6                  -- image format
    # comment           -- comment lines begin with
    ## another comment  -- any number of comment lines
    200 300             -- image width & height
    255                 -- max color value

 Check if the image format is P6. If not, print invalid format error message.
 If there are comments in the file, skip them. You may assume that comments exist only in the header block.
 Read the image size information and store them in width and height.
 Check the rgb component, if not 255, display error message.
 Return: pointer to PPMPixel that has the pixel data of the input image (filename).The pixel data is stored in scanline order from left to right (up to bottom) in 3-byte chunks (r g b values for each pixel) encoded as binary numbers.
 */
PPMPixel *read_image(const char *filename, unsigned long int *width, unsigned long int *height)
{
    char buff[16];
    char c;
    FILE *fp = fopen(filename, "rb");

    if (!fp)
    {
        printf("Error: Unable to open file for reading.\n");
        exit(1);
    }

    if (!fgets(buff, sizeof(buff), fp))
    {
        printf("Error: Invalid image format.\n");
        exit(1);
    }

    if (buff[0] != 'P' || buff[1] != '6')
    {
        printf("Error: Invalid image format. Only P6 format is supported.\n");
        exit(1);
    }

    c = getc(fp);
    while (c == '#')
    {
        while (getc(fp) != '\n')
            ;         // Skip the comment line
        c = getc(fp); // Check if the next line also starts with '#'
    }

    ungetc(c, fp); // Put back the read character if not '#'

    int rgb_comp_color;
    if (fscanf(fp, "%lu %lu %d%*c", width, height, &rgb_comp_color) != 3)
    {
        printf("Error: Invalid image size (width, height) or rgb component.\n");
        exit(1);
    }

    if (rgb_comp_color != RGB_COMPONENT_COLOR)
    {
        printf("Error: Invalid rgb component. Only 255 is supported.\n");
        exit(1);
    }

    PPMPixel *img = (PPMPixel *)malloc(sizeof(PPMPixel) * (*width) * (*height));
    fread(img, sizeof(PPMPixel), (*width) * (*height), fp);
    fclose(fp);

    return img;
}

/* The thread function that manages an image file.
 Read an image file that is passed as an argument at runtime.
 Apply the Laplacian filter.
 Update the value of total_elapsed_time.
 Save the result image in a file called laplaciani.ppm, where i is the image file order in the passed arguments.
 Example: the result image of the file passed third during the input shall be called "laplacian3.ppm".

*/
void *manage_image_file(void *args)
{
    struct file_name_args *fargs = (struct file_name_args *)args;

    unsigned long int width, height;
    PPMPixel *image = read_image(fargs->input_file_name, &width, &height);

    double elapsedTime;
    PPMPixel *result = apply_filters(image, width, height, &elapsedTime);

    total_elapsed_time += elapsedTime;

    write_image(result, fargs->output_file_name, width, height);

    free(image);
    free(result);

    return NULL;
}
/*The driver of the program. Check for the correct number of arguments. If wrong print the message: "Usage ./a.out filename[s]"
  It shall accept n filenames as arguments, separated by whitespace, e.g., ./a.out file1.ppm file2.ppm    file3.ppm
  It will create a thread for each input file to manage.
  It will print the total elapsed time in .4 precision seconds(e.g., 0.1234 s).
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage ./edge_detector filename[s]\n");
        return 1;
    }

    int num_images = argc - 1;
    pthread_t threads[num_images];
    struct file_name_args args[num_images];

    for (int i = 0; i < num_images; i++)
    {
        args[i].input_file_name = argv[i + 1];
        sprintf(args[i].output_file_name, "laplacian%d.ppm", i + 1);

        if (pthread_create(&threads[i], NULL, manage_image_file, (void *)&args[i]) != 0)
        {
            printf("Error: Unable to create thread.\n");
            return 1;
        }
    }

    for (int i = 0; i < num_images; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("Total elapsed time: %.4f s\n", total_elapsed_time);

    return 0;
}
