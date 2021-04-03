// Christine Par
// 1001490910
// Assignment 2

#include "bitmap.h"
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h> // added this header so i can time the process


int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max, int userThreads); // added last parameter for user input num of threads
void *foo (void *ptr); // for thread function to be called inside compute_image

void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int 	 userThreads = 1; // default for number of threads will be 1 if user did not specify

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:n:o:h"))!=-1)  // added :n for userThreads
	{
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'n':											// add 'n' for number of threads user wants to run
				userThreads = atof(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;

		}
	}

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d threads=%d outfile=%s\n",xcenter,ycenter,scale,max,userThreads,outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the Mandelbrot image
	compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max,userThreads);

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}
	return 0;
}

// structure so i can use the variables inside foo since i can't just pass them inside foo by themselves
typedef struct fooStruct
{
	struct bitmap *bm;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int max;
	int lineStarts;
	int lineEnds;
	int k;
	int userThreads;
} fooStruct;

/*
Turned the for loop that was in compute_image into a function that will be for the threads to run
-will do calculations that threads will do-
*/

void *foo (void * ptr)
{
	fooStruct* myFoo = (fooStruct*) ptr;	// declaring a type fooStruct pointer
	int width = bitmap_width(myFoo->bm);
	int height = bitmap_height(myFoo->bm);
	int i,j;

	for(j = myFoo->lineStarts; j <= myFoo->lineEnds; j++)		// loop will run until the end of the line for that thread
	{
		for(i=1; i <= width; i++)
		{
			// Determine the point in x,y space for that pixel.
			double x = myFoo->xmin + i*(myFoo->xmax-myFoo->xmin)/width;
			double y = myFoo->ymin + j*(myFoo->ymax-myFoo->ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,myFoo->max);

			// Set the pixel in the bitmap.
			bitmap_set(myFoo->bm,i,j,iters);
		}
	}
	return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/
void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max, int userThreads)
{
	int linesPerThread = bitmap_height(bm)/userThreads;			// how many lines one thread will run
	pthread_t* myThread = malloc (userThreads * sizeof(pthread_t));
	fooStruct* myStruct = malloc (userThreads * sizeof(fooStruct));
	int i;

	/*
	this for loop will iterate from 0 to however many threads the user specified
	the threads will be created in this loop 
	*/
	for(i = 0; i < userThreads; i++)
	{
		myStruct[i].bm = bm;                    // we have to assign those items in the struct to
		myStruct[i].xmin = xmin;								 // actual numbers so this is what im doing
		myStruct[i].xmax = xmax;
		myStruct[i].ymin = ymin;
		myStruct[i].ymax = ymax;
		myStruct[i].max = max;
		myStruct[i].userThreads = userThreads;
		myStruct[i].lineStarts = i * linesPerThread +1;

		if(i == userThreads -1)												// depending on the iteration
		{																							// the last line of thread for that
			myStruct[i].lineEnds = bitmap_height(bm);	  // perticular thread is determined
		}																							// hence if and else
		else
		{
			myStruct[i].lineEnds = (i+1) * linesPerThread;
		}

		pthread_create(&myThread[i], NULL, foo, &myStruct[i]); // creating threads
		//pthread_join(myThread[i], NULL);						// implementing join thread here didn't run well
	}

	for(i = 0; i < userThreads; i++)								// here the myThreads will join together
	{																								// for as many threads the user specified
		pthread_join(myThread[i], NULL);
	}
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}
