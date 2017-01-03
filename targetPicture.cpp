#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <random>
#include <string.h>
#include <fstream>

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
uint32_t scoreFilter(double filter[], size_t filterSize,
                     pixel source[], pixel target[], int32_t imgSize);

// Returns the pixel in the position (row, col) of the filtered image
pixel filterPix(double filter[], size_t filterSize,
                int32_t row, int32_t col,
                pixel source[], int32_t imgSize);

// Prints a list of filter_t's
void printFilters(filter_t filters[], size_t filterSize, size_t len);

// Mutates best filters to make new list of filters
void mutate(filter_t best[], size_t numBest,
            filter_t newFilters[], size_t numFilters,
            size_t filterSize);

void drawImg(pixel* img, size_t imgSize);

int main(int argc, char *argv[]){
  // Default values
  size_t numGenerations = 1000;
  size_t filterSize = 3; // length of one side of the filter
  size_t numBest = 16;
  size_t numFilters = 256;

  int argi = 1;
  while(argi < argc){
    if(strcmp(argv[argi], "-h") == 0){
      // Print contents of help.txt and exit
      string line;
      ifstream helpFile("help.txt");
      if (!helpFile.is_open()){
        cout << "Error: couldn't find help\n";
        return 1;
      }
      while(getline(helpFile, line)){
        cout << line << "\n";
      }
      helpFile.close();
      return 1;
    } else if(strcmp(argv[argi], "-g") == 0){
      numGenerations = (size_t) stoi(argv[argi + 1]);
      argi += 2;
    } else if(strcmp(argv[argi], "-f") == 0){
      filterSize = (size_t) stoi(argv[argi + 1]);
      argi += 2;
    } else if(strcmp(argv[argi], "-b") == 0){
      numBest = (size_t) stoi(argv[argi + 1]);
      argi += 2;
    } else if(strcmp(argv[argi], "-n") == 0){
      numFilters = (size_t) stoi(argv[argi + 1]);
      argi += 2;
    } else if(argv[argi][0] == '-'){
      cout << "Invalid option " << argv[argi];
      cout << ". Try -h for more information.\n";
      return 1;
    } else {
      break;
    }
  }

  pixel *source;
  pixel *target;
  size_t imgSize;
  if(argi < argc){
    // Print contents of help.txt and exit
    string word;
    ifstream inFile(argv[argi]);
    if (!inFile.is_open()){
      cout << "Error: couldn't open " << argv[argi] << "\n";
      return 1;
    }
    inFile >> word; // Get special number to make sure file is of correct type
    if(word.compare("646B") != 0){
      cout << "Error: invalid format: " << argv[argi] << "\n";
      return 1;
    }
    inFile >> word; // Get image size
    imgSize = (size_t) stoi(word);
    source = new pixel[imgSize * imgSize];
    for(size_t i = 0; i < imgSize * imgSize; i++){
      inFile >> word;
      source[i] = (unsigned char) (unsigned int) stoi(word);
    }
    inFile.close();

    if(argc - argi == 1){
      cout << "No target image specified. Using source as target.\n";
      target = source;
    } else {
      inFile.open(argv[argi+1]);
      if (!inFile.is_open()){
        cout << "Error: couldn't open " << argv[argi] << "\n";
        return 1;
      }
      inFile >> word; // Get special number to make sure file is of correct type
      if(word.compare("646B") != 0){
        cout << "Error: invalid format: " << argv[argi] << "\n";
        return 1;
      }
      inFile >> word; // Get image size
      if((size_t) stoi(word) != imgSize){
        cout << "Error: Source and target are not the same size\n";
        return 1;
      }
      target = new pixel[imgSize * imgSize];
      for(size_t i = 0; i < imgSize * imgSize; i++){
        inFile >> word;
        target[i] = (unsigned char) (unsigned int) stoi(word);
      }
      inFile.close();
    }

  } else { // Use a default source and target
    imgSize = 3;
    source = new pixel[9];
    source[0] = 50;
    source[1] = 60;
    source[2] = 80;
    source[3] = 60;
    source[4] = 100;
    source[5] = 90;
    source[6] = 80;
    source[7] = 90;
    source[8] = 75;
    target = source;
  }

  // Set up random
  srand(time(NULL));

  filter_t *filters = new filter_t[numFilters];
  for(size_t i = 0; i < numFilters; i++){
    filters[i] = new struct filter_header;
    // A filter is an array of doubles
    filters[i]->filter = new double[filterSize * filterSize];
    // Randomly populate each filter
    for(size_t j = 0; j < filterSize * filterSize; j++){
      // Favors positive numbers and low numbers
      filters[i]->filter[j] =
        log((double)rand()/RAND_MAX) * (rand() % 3 == 0 ? 1 : -1);
    }
  }

  // Ordered list of best (lowest) 16 filters
  filter_t *best = new filter_t[numBest];
  size_t numFilled = 0;
  for(size_t gen = 0; gen < numGenerations; gen++){
    for(size_t i = 0; i < numFilters; i++){
      filters[i]->score = scoreFilter(filters[i]->filter, filterSize,
                                      source, target, imgSize);

      if(numFilled < numBest || filters[i]->score < best[numBest - 1]->score) {
        // Insertion sort filters to only ever keep top numBest
        filter_t temp;
        int insertIndex;

        if(numFilled < numBest){
          best[numFilled] = filters[i];
          insertIndex = numFilled;
          numFilled++;
        } else {
          delete[] best[numBest - 1]->filter;
          delete best[numBest - 1];
          best[numBest - 1] = filters[i];
          insertIndex = numBest - 1;
        }

        while(insertIndex > 0 &&
              best[insertIndex]->score < best[insertIndex - 1]->score){
          // LOOP INVARIANTS
          // best[0, insertIndex) < best(insertIndex, numBest - 1]
          // best is sorted on [0, insertIndex) and [insertIndex, numBest - 1]
          // insertIndex >= 0

          // Swap best[insertIndex] and best[insertIndex - 1]
          temp = best[insertIndex - 1];
          best[insertIndex - 1] = best[insertIndex];
          best[insertIndex] = temp;
          insertIndex--;
        }

      } else { // Score is not one of the best
        delete[] filters[i]->filter;
        delete filters[i];
      }
    }
    mutate(best, numBest, filters, numFilters, filterSize);
  }
  cout << "\nSource:\n";
  drawImg(source, imgSize);
  cout << "\nTarget:\n";
  drawImg(target, imgSize);
  cout << "\nTop 3:\n";
  printFilters(best, filterSize, 3); // Print top 3 filters and scores

  // Free everything
  for(size_t i = 0; i < numBest; i++){
    delete[] best[i]->filter;
    delete best[i];
  }
  for(size_t i = 0; i < numFilters; i++){
    delete[] filters[i]->filter;
    delete filters[i];
  }
  delete[] best;
  delete[] filters;
  if(target == source){ // If both point to the same thing...
    delete[] source; // ... only delete one
  } else {
    delete[] source;
    delete[] target;
  }
}

// Scores the filter based on how accurately the filtered source matches the
// target. A lower value is a better score (more similar).
uint32_t scoreFilter(double filter[], size_t filterSize,
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
pixel filterPix(double filter[], size_t filterSize,
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
void printFilters(filter_t filters[], size_t filterSize, size_t len){
  for(size_t i = 0; i < len; i++){
    cout << i << ":\n  Score: " << filters[i]->score << "\n";
    cout << "  Filter:\n";
    for(size_t r = 0; r < filterSize; r++){
      cout << "    ";
      for(size_t c = 0; c < filterSize; c++){
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
            size_t filterSize){
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

void drawImg(pixel* img, size_t imgSize){
  for(size_t r = 0; r < imgSize; r++){
    for(size_t c = 0; c < imgSize; c++){
      cout << (unsigned int) img[r * imgSize + c] << " ";
    }
    cout << "\n";
  }
}
