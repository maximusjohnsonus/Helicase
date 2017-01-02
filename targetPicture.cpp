#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <random>

using namespace std;

/*typedef uint32_t pixel; //XXXXXXXXRRRRRRRRGGGGGGGGBBBBBBBB (no alpha)
  uint32_t getRed(pixel p){
  return (p >> 16) & 0xFF;
  }
  uint32_t getGreen(pixel p){
  return (p >> 8) & 0xFF;
  }
  uint32_t getBlue(pixel p){
  return p & 0xFF;
  }

  int comparePixel(pixel p, pixel q){
  return abs(getRed(p) - getRed(q)) +
  abs(getGreen(p) - getGreen(q)) +
  abs(getBlue(p) - getBlue(q));
  }*/

typedef unsigned char pixel; // Only B&W
typedef struct filter_header *filter_t;
struct filter_header {
  double *filter;
  uint32_t score;
};

// Scores the filter based on how accurately the filtered source matches the
// target. A lower value is a better score (more similar).
uint32_t scoreFilter(double filter[], unsigned char filterSize,
                     pixel source[], pixel target[], int32_t imgSize);

// Returns the pixel in the position (row, col) of the filtered image
pixel filterPix(double filter[], unsigned char filterSize,
                int32_t row, int32_t col,
                pixel source[], int32_t imgSize);

// Prints a list of filter_t's
void printFilters(filter_t filters[], unsigned char filterSize, size_t len);

// Mutates best filters to make new list of filters
void mutate(filter_t best[], size_t numBest,
            filter_t newFilters[], size_t numFilters,
            unsigned char filterSize);

int main(){
  // Random setup
  srand(time(NULL));

  filter_t *filters = new filter_t[64]; // filters is an array of 64 filter_t's
  for(int i = 0; i < 64; i++){
    filters[i] = new struct filter_header;
    filters[i]->filter = new double[9]; // a filter is an array of 9 doubles
    // Randomly populate each filter
    for(int j = 0; j < 9; j++){
      // Favors positive numbers and low numbers
      filters[i]->filter[j] =
        log((double)rand()/RAND_MAX) * (rand() % 3 == 0 ? 1 : -1);
    }
  }
  pixel source[] = {87,  107, 117, 129, 130,
                    74,  111, 135, 149, 187,
                    89,  127, 200, 195, 182,
                    111, 154, 195, 176, 170,
                    130, 190, 187, 172, 160};
  // Ordered list of best (lowest) 16 filters
  filter_t *best = new filter_t[16];
  size_t numFilled = 0;
  for(int runs = 0; runs < 1000; runs++){
    for(int i = 0; i < 64; i++){
      filters[i]->score = scoreFilter(filters[i]->filter, 3, source, source, 5);

      if(numFilled < 16 || filters[i]->score < best[15]->score) {
        // Insertion sort filters to only ever keep top 16
        filter_t temp;
        int insertIndex;

        if(numFilled < 16){
          best[numFilled] = filters[i];
          insertIndex = numFilled;
          numFilled++;
        } else {
          delete[] best[15]->filter;
          delete best[15];
          best[15] = filters[i];
          insertIndex = 15;
        }

        while(insertIndex > 0 &&
              best[insertIndex]->score < best[insertIndex - 1]->score){
          // LOOP INVARIANTS
          // best[0, insertIndex) < best(insertIndex, 15]
          // best is sorted on [0, insertIndex) and on [insertIndex, 15]
          // insertIndex >= 0

          // Swap best[insertIndex] and best[insertIndex - 1]
          temp = best[insertIndex - 1];
          best[insertIndex - 1] = best[insertIndex];
          best[insertIndex] = temp;
          insertIndex--;

        }
      } else { // Score is not in best 16
        delete[] filters[i]->filter;
        delete filters[i];
      }
    }
    printFilters(best, 3, 3); // Print top 3
    cout << "\n";
    //    printFilters(filters, 3, 5);
    mutate(best, 16, filters, 64, 3);
    //    printFilters(filters, 3, 5);

  }

  // Free everything
  for(int i = 0; i < 16; i++){
    delete[] best[i]->filter;
    delete best[i];
  }
  for(int i = 0; i < 64; i++){
    delete[] filters[i]->filter;
    delete filters[i];
  }
  delete[] best;
  delete[] filters;
}

// Scores the filter based on how accurately the filtered source matches the
// target. A lower value is a better score (more similar).
uint32_t scoreFilter(double filter[], unsigned char filterSize,
                     pixel source[], pixel target[], int32_t imgSize){
  assert(filterSize % 2 == 1);
  uint32_t score = 0;
  pixel filteredPix;
  for(int32_t r = 0; r < imgSize; r++){
    for(int32_t c = 0; c < imgSize; c++){
      filteredPix = filterPix(filter, filterSize, r, c, source, imgSize);

      // Equivalent to abs(filteredPix - target[r][c]) but works with ubytes
      if(filteredPix > target[r*imgSize + c]){
        score += (uint32_t) (filteredPix - target[r*imgSize + c]);
      } else {
        score += (uint32_t) (target[r*imgSize + c] - filteredPix);
      }
    }
  }
  return score;
}

// Returns the pixel in the position (row, col) of the filtered image
pixel filterPix(double filter[], unsigned char filterSize,
                int32_t row, int32_t col,
                pixel source[], int32_t imgSize){
  unsigned int pixelVal;
  pixelVal = 0;
  for(char fr = -1 * (char)(filterSize/2); fr <= (char)(filterSize/2); fr++){
    for(char fc = -1 * (char)(filterSize/2); fc <= (char)(filterSize/2); fc++){
      if(0 <= row + fr && row + fr < imgSize &&
         0 <= col + fc && col + fc < imgSize){
        pixelVal += source[(row + fr)*imgSize + (col + fc)] *
          filter[(fr + filterSize/2)*filterSize + (fc + filterSize/2)];
      }
    }
  }
  if(pixelVal >= 255){
    return (unsigned char) 255;
  } else if (pixelVal <= 0){
    return (unsigned char) 0;
  } else {
    return (unsigned char) pixelVal;
  }
}

// Prints a list of filter_t's
void printFilters(filter_t filters[], unsigned char filterSize, size_t len){
  for(size_t i = 0; i < len; i++){
    cout << i << ":\n  Score: " << filters[i]->score << "\n";
    cout << "  Filter:\n";
    for(unsigned char r = 0; r < filterSize; r++){
      cout << "    ";
      for(unsigned char c = 0; c < filterSize; c++){
        cout << filters[i]->filter[r * filterSize + c] << "\t";
      }
      cout << "\n";
    }
    cout << "\n";
  }
}

// Mutates best filters to make new list of filters
void mutate(filter_t best[], size_t numBest,
            filter_t newFilters[], size_t numFilters,
            unsigned char filterSize){
  default_random_engine generator;
  normal_distribution<double> normal(0.0, 0.05);
  assert(numBest <= numFilters);

  // Fill with mutations
  for(size_t i = 0; i < numFilters; i++){
    newFilters[i] = new struct filter_header;
    newFilters[i]->filter = new double[9];
    for(size_t j = 0; j < ((size_t)filterSize)*((size_t)filterSize); j++){
      newFilters[i]->filter[j] = best[i % numBest]->filter[j] +
        normal(generator);
    }
  }
}
