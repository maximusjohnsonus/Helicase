#include <assert.h>
#include <stdint.h>
#include <iostream>

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
// Scores the filter based on how accurately the filtered source matches the
// target. A lower value is a better score (more similar).
unsigned long scoreFilter(double filter[], unsigned char filterSize,
                          pixel source[], pixel target[], int32_t imgSize);

// Returns the pixel in the position (row, col) of the filtered image
pixel filterPix(double filter[], unsigned char filterSize,
                int32_t row, int32_t col,
                pixel source[], int32_t imgSize);

int main(){
  double filter[] = {0, 0, 0, 1, 0, 0, 0, 0, 0};
  pixel source[] = {1,   2,   3,   4,   5,
                    213, 54,  26,  63,  231,
                    0,   0,   0,   123, 111,
                    0,   1,   3,   7,   15,
                    31,  63,  127, 255, 0};
  unsigned long score = scoreFilter(filter, 3, source, source, 5);
  // delete(filter);
  // delete(source);
  (void)score;
  cout << score << "\n";
}

// Scores the filter based on how accurately the filtered source matches the
// target. A lower value is a better score (more similar).
unsigned long scoreFilter(double filter[], unsigned char filterSize,
                          pixel source[], pixel target[], int32_t imgSize){
  assert(filterSize % 2 == 1);
  unsigned long score = 0;
  pixel filteredPix;
  for(int32_t r = 0; r < imgSize; r++){
    for(int32_t c = 0; c < imgSize; c++){
      filteredPix = filterPix(filter, filterSize, r, c, source, imgSize);

      // Equivalent to abs(filteredPix - target[r][c]) but works with ubytes
      if(filteredPix > target[r*imgSize + c]){
        score += (unsigned long) (filteredPix - target[r*imgSize + c]);
      } else {
        score += (unsigned long) (target[r*imgSize + c] - filteredPix);
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
