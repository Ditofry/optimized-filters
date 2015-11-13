#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"
// Reference for why I'm inlining here:
// http://www.drdobbs.com/the-new-c-inline-functions/184401540
// This could be a more significant optimization if I were using
// functions in my loops, though it should also be noted that since
// it's repeatedly adding function code inline at call sites that
// it would grow the size of the binary.
inline int Filter::get(int r, int c){return data[ r * dim + c ];}

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;

  // There are a few things that we can cache outside of the loop
  float divisor = filter->getDivisor();
  int width = input->width, height = input->height, size = filter->getSize();

  // This is not very readable, but we CAN optimize this program by caching
  // our filter data here
  int get00 = filter->get(0,0); //avoid running get() inside loops, just use pre-declared variables
  int get01 = filter->get(0,1);
  int get02 = filter->get(0,2);
  int get10 = filter->get(1,0);
  int get11 = filter->get(1,1);
  int get12 = filter->get(1,2);
  int get20 = filter->get(2,0);
  int get21 = filter->get(2,1);
  int get22 = filter->get(2,2);

  for(int row = 1; row <= height; row++) {
    for(int col = 1; col <= width; col++) {

      int tRed = 0;
      int tGreen = 0;
      int tBlue = 0;

      // This is how I wish I could leave it :(  This gave me a B+ in terms
      // of scoring.
      // for (int i = 0; i < size; i++) {
      //   for (int j = 0; j < size; j++) {
      //     tRed += input->color[row + i - 1][col + j - 1][COLOR_RED] * filter->get(i, j);
      //     tGreen += input->color[row + i - 1][col + j - 1][COLOR_GREEN] * filter->get(i, j);
      //     tBlue += input->color[row + i - 1][col + j - 1][COLOR_BLUE] * filter->get(i, j);
    	//   }
    	// }

      // We know that the inner loops go from 0 to 3.
      // Looking at color as an XYZ 3d matrix, we itterate j, then i
      // i = 0
      tRed += input->color[0][0][COLOR_RED] * get00;
      tGreen += input->color[0][0][COLOR_GREEN] * get00;
      tBlue += input->color[0][0][COLOR_BLUE] * get00;

      tRed += input->color[0][1][COLOR_RED] * get01;
      tGreen += input->color[0][1][COLOR_GREEN] * get01;
      tBlue += input->color[0][1][COLOR_BLUE] * get01;

      tRed += input->color[0][2][COLOR_RED] * get02;
      tGreen += input->color[0][2][COLOR_GREEN] * get02;
      tBlue += input->color[0][2][COLOR_BLUE] * get02;

      // i = 1
      tRed += input->color[1][0][COLOR_RED] * get10;
      tGreen += input->color[1][0][COLOR_GREEN] * get10;
      tBlue += input->color[1][0][COLOR_BLUE] * get10;

      tRed += input->color[1][1][COLOR_RED] * get11;
      tGreen += input->color[1][1][COLOR_GREEN] * get11;
      tBlue += input->color[1][1][COLOR_BLUE] * get11;

      tRed += input->color[1][2][COLOR_RED] * get12;
      tGreen += input->color[1][2][COLOR_GREEN] * get12;
      tBlue += input->color[1][2][COLOR_BLUE] * get12;

      // i = 2
      tRed += input->color[2][0][COLOR_RED] * get20;
      tGreen += input->color[2][0][COLOR_GREEN] * get20;
      tBlue += input->color[2][0][COLOR_BLUE] * get20;

      tRed += input->color[2][1][COLOR_RED] * get21;
      tGreen += input->color[2][1][COLOR_GREEN] * get21;
      tBlue += input->color[2][1][COLOR_BLUE] * get21;

      tRed += input->color[2][2][COLOR_RED] * get22;
      tGreen += input->color[2][2][COLOR_GREEN] * get22;
      tBlue += input->color[2][2][COLOR_BLUE] * get22;
      /*** END LOOP UNROLL ***/

      tRed /= divisor;
      tGreen /= divisor;
      tBlue /= divisor;

      if (tRed < 0){tRed = 0;}
      if (tRed > 255){tRed = 255;}
      if (tGreen < 0){tGreen = 0;}
      if (tGreen > 255){tGreen = 255;}
      if (tBlue < 0){tBlue = 0;}
      if (tBlue > 255){tBlue = 255;}

      output->color[row][col][COLOR_RED] = tRed;
      output->color[row][col][COLOR_GREEN] = tGreen;
      output->color[row][col][COLOR_BLUE] = tBlue;
    }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
