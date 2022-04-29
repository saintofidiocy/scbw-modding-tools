#include <stdio.h>
#include <stdlib.h>
#include "SpkComp.h"
#include <windows.h>

#define MAX_LAYER_COUNT  20  // sc default is 5 -- perhaps this should increase dynamically
#define LAYER_WIDTH      648
#define LAYER_HEIGHT     488
#define MAX_STAR_DIM     8

#define LAYER_OFFSET     (LAYER_HEIGHT + 2)
#define BUFFER_HEIGHT    (LAYER_OFFSET * MAX_LAYER_COUNT)

typedef struct {
  u16 x,y;
  u32 imgOffs;
} starRef;

typedef struct {
  u16 w,h;
  u8 data[MAX_STAR_DIM * MAX_STAR_DIM];
} starData;

/*typedef struct {
  u8 b,g,r,a;
} RGBQUAD;*/


typedef struct {
  u32 id;
  u16 x1,y1;
  u16 x2,y2;
  u16 w,h;
  u32 hash;
  s32 nextDuplicate;
  s32 parent;
} region;

typedef struct {
  u16 x,y; // relative to region
  u16 w,h;
  u32 offset;
  u32 hash;
  u8 data[MAX_STAR_DIM][MAX_STAR_DIM];
} tmpStar;

typedef struct {
  u32 count;
  u32 max; // (w / MAX_STAR_DIM + 1) * (h / MAX_STAR_DIM + 1)
  u32 firstIndex;
  tmpStar stars[1];
} starGroup;

typedef struct{
  u32 size;
  s32 width;
  s32 height;
  u16 planes;
  u16 bpp;
  u32 compression;
  u32 imgSize;
  u32 hres;
  u32 vres;
  u32 palSize;
  u32 icolors;
  RGBQUAD pal[256];
} BMI;

typedef struct __attribute__((packed)) {
  u16 id; // "BM"
  u32 filesize;
  u32 unused;
  u32 dataoffs;
  BMI bmi;
  u8 data[1];
} bmp;


RGBQUAD pal[256];

u8 src[BUFFER_HEIGHT][LAYER_WIDTH];
s32 rgnFinder[BUFFER_HEIGHT][LAYER_WIDTH];

u32 rgnCount = 0;
u32 rgnMax = 0;
region* regions = NULL;
starGroup** regionStars = NULL;

u16 layerCount = 0;
u16 starCounts[MAX_LAYER_COUNT] = {0};
u32 offsets[MAX_LAYER_COUNT] = {0};

u32 bitmapCount = 0;
u32 bitmapMax = 0;
starData* bitmaps = NULL;

u32 starTotal = 0;
u32 starMax = 0;
starRef* stars = NULL;


u8 input[MAX_LAYER_COUNT][260] = {0};
u8 output[260] = "";
u32 inputCount = 0;

bool fileExists(const char* filename);
char* getExtension(char* filename);

void compile();
u32 readSrcImage(const char* filename, u32 layerID);
void findRegions();
void rgnFill(region* r, u32 x, u32 y);
void getRgnHash(region* r);
void findDuplicates();
void parseRegions();
void findSubImages(region* r, starGroup* s);

void decompile();


int main(int argc, char *argv[]){
  u32 i;
  
  puts("SPK Compiler v" VSTR "\nBy O)FaRTy1billion\n");
  
  memset(src, 0, LAYER_WIDTH * BUFFER_HEIGHT);
  for(i = 0; i < MAX_LAYER_COUNT; i++){
    starCounts[i] = 0;
    offsets[i] = 0;
  }
  layerCount = 0;
  
  if(argc == 1){
    puts("No input file specified.\n"
/*       "\n"
         "Give your layer images sequential names (i.e. layer0.bmp, layer1.bmp,\n"
         "layer2.bmp, etc.) and drag & drop the first on to the EXE.\n"
         "Alternatively, using the command-line you can list the layers explicitly.\n"*/
         "\n"
         "Images must be 8-bit bitmaps with no compression and the appropriate palette.\n"
         "Layers are 648x488. Smaller images will be positioned in the top left corner\n"
         "and larger images will be clipped.\n");
    system("pause");
    return 0;
  }
  
  bool compileMode = false;
  char* ext = NULL;
  u32 len;
  
  inputCount = 0;
  for(i = 1; i < argc; i++){
    ext = getExtension(argv[i]);
    if(!strcmpi(ext, ".bmp") && inputCount < MAX_LAYER_COUNT){
      if(i == 1){
        compileMode = true;
        strcpy(output, argv[i]);
        len = strlen(output) - 4; // remove ".bmp"
        for(; len > 0 && output[len-1] >= '0' && output[len-1] <= '9'; len--); // remove numbers
        strcpy(&output[len], ".spk");
      }
      strcpy(input[inputCount], argv[i]);
      inputCount++;
    }else if(!strcmpi(ext, ".spk")){
      if(i == 1){
        compileMode = false;
      }
      strcpy(output, argv[i]);
    }else{
      printf("Unknown file or argument \"%s\"\n", argv[i]);
    }
  }
  
  
  if((compileMode && (inputCount == 0 || input[0][0] == 0)) || output[0] == 0){
    puts("\nNo files specified.\n");
  }else if(compileMode){
    compile();
  }else{
    decompile();
  }
  
  system("PAUSE");
  return 0;
}

char* getExtension(char* filename){
  int i;
  int len = strlen(filename);
  for(i = len - 1; i >=0 && filename[i] != '\\'; i--){
    if(filename[i] == '.') return &filename[i];
  }
  return &filename[len];
}

bool fileExists(const char* filename){
  u32 r = GetFileAttributes(filename);
  return r != INVALID_FILE_ATTRIBUTES && (r & FILE_ATTRIBUTE_DIRECTORY) == 0;
}


void compile(){
  u32 i,j,x,y,w,h,offset,layer;
  starData* star = NULL;
  char* ext = NULL;
  
  x = 0;
  for(i = 0; i < inputCount; i++){
    if(readSrcImage(input[i], i)){
      x++;
    }
    layerCount++;
  }
  
  // Find additional sequential images
  if(i < MAX_LAYER_COUNT){
    j = inputCount - 1;
    ext = (u8*)(getExtension(input[j]) - 1);
    if(*ext >= '0' && *ext <= '9'){ // is it number before the extension?
      for( ; i < MAX_LAYER_COUNT; i++){
        (*ext)++; // increment file number
        if(fileExists(input[j])){
          if(readSrcImage(input[j], i)){
            x++;
          }
          layerCount++;
        }else{
          break; // no more files
        }
      }
    }
  }
  
  if(layerCount == 0){
    puts("\nERROR: Coult not load any files.\n");
    system("pause");
    return;
  }
  if(x != 0){
    puts("\nWARNING: There were errors loading images. SPK may be incomplete.\n");
  }
  
  findRegions();
  findDuplicates();
  parseRegions();
  
  for(i = 0; i < rgnCount; i++){
    layer = regions[i].y1 / LAYER_OFFSET;
    if(regionStars[i] != NULL){
      regionStars[i]->firstIndex = bitmapCount;
      bitmapCount += regionStars[i]->count;
      starTotal += regionStars[i]->count;
      starCounts[layer] += regionStars[i]->count;
    }else{
      starTotal += regionStars[regions[i].parent]->count;
      starCounts[layer] += regionStars[regions[i].parent]->count;
    }
  }
  
  printf("Number of Stars: %d\n", starTotal);
  
  offset = 0;
  for(i = 0; i < layerCount; i++){
    offsets[i] = offset;
    offset += starCounts[i];
  }
  printf("Number of Stars Check: %d\n", offset);
  
  bitmaps = malloc(sizeof(starData) * bitmapCount);
  stars = malloc(sizeof(starRef) * starTotal);
  offset = 2 + layerCount * 2 + sizeof(starRef) * starTotal;
  
  for(i = 0; i < rgnCount; i++){
    if(regionStars[i] != NULL){
      for(j = 0; j < regionStars[i]->count; j++){
        w = regionStars[i]->stars[j].w;
        h = regionStars[i]->stars[j].h;
        regionStars[i]->stars[j].offset = offset;
        star = &bitmaps[regionStars[i]->firstIndex + j];
        star->w = w;
        star->h = h;
        for(y = 0; y < h; y++){
          for(x = 0; x < w; x++){
            star->data[y*w + x] = regionStars[i]->stars[j].data[y][x];
          }
        }
        offset += 4 + w*h;
      }
    }
  }
  
  x = 0;
  for(i = 0; i < rgnCount; i++){
    j = i;
    if(regionStars[i] == NULL){
      j = regions[i].parent;
    }
    
    for(y = 0; y < regionStars[j]->count; y++){
      stars[x].x = regions[i].x1 + regionStars[j]->stars[y].x;
      stars[x].y = (regions[i].y1 % LAYER_OFFSET) + regionStars[j]->stars[y].y;
      stars[x].imgOffs = regionStars[j]->stars[y].offset;
      x++;
    }
  }
  
  
  printf("\nWriting %s ...\n\n", output);
  FILE * pFile = fopen(output, "wb");
  fwrite(&layerCount, sizeof(u16), 1, pFile);
  fwrite(starCounts, sizeof(u16), layerCount, pFile);
  for(i = layerCount; i > 0; i--){ // star layers get added in reverse order
    fwrite(&stars[offsets[i-1]], sizeof(starRef), starCounts[i-1], pFile);
  }
  for(i = 0; i < bitmapCount; i++){
    fwrite(&bitmaps[i], sizeof(u16), 2, pFile);
    fwrite(bitmaps[i].data, bitmaps[i].w * bitmaps[i].h, 1, pFile);
  }
  fclose(pFile);
  
  puts("Done");
  
  // clean up
  for(i=0; i<rgnCount;i++){
    if(regionStars[i] != NULL) free(regionStars[i]);
  }
  free(regionStars);
  free(regions);
  free(bitmaps);
  free(stars);
}


u32 readSrcImage(const char* filename, u32 layerID){
  u32 size = 0;
  bmp* bmp = NULL;
  u8* data = NULL;
  printf("Reading %s ...\n", filename);
  FILE * pFile = fopen(filename, "rb");
  if(pFile == NULL){
    printf("ERROR: Could not open file %s\n", filename);
    return 1;
  }
  fseek(pFile, 0, SEEK_END);
  size = ftell(pFile);
  rewind(pFile);
  if(size == 0){
    printf("ERROR: Could not read file %s\n", filename);
    fclose(pFile);
    return 1;
  }
  bmp = malloc(size);
  if(bmp == NULL){
    puts("ERROR: Memory");
    fclose(pFile);
    return 1;
  }
  fread(bmp, size, 1, pFile);
  fclose(pFile);
  if(bmp->id != 0x4D42 || bmp->bmi.bpp != 8 || bmp->bmi.compression != 0){
    printf("ERROR: Invalid file format %s\n", filename);
    puts("Must be 8-bit BMP, preferably with the space platform palette.");
    free(bmp);
    return 1;
  }
  data = (u8*)(bmp) + bmp->dataoffs;
  u32 x,y;
  u32 w,h;
  
  bool invertX = bmp->bmi.width < 0;
  bool invertY = bmp->bmi.height > 0;
  w = invertX ? -bmp->bmi.width : bmp->bmi.width;
  h = invertY ? bmp->bmi.height : -bmp->bmi.height;
  u32 linewidth = (w + 3) & ~3;
  
  u32 line = 0;
  if(invertY){
    line = (h-1) * linewidth;
    linewidth = -linewidth;
  }
  
  if(w != LAYER_WIDTH || h != LAYER_HEIGHT){
    printf("WARNING: Image does not match parallax layer dimensions (%dx%d) and may be clipped.\n", LAYER_WIDTH, LAYER_HEIGHT);
  }
  
  for(y = 0; y < h && y < LAYER_HEIGHT; y++){
    for(x = 0; x < w && x < LAYER_WIDTH; x++){
      if(invertX){
        src[LAYER_OFFSET * layerID + y][x] = data[line + w - x - 1];
      }else{
        src[LAYER_OFFSET * layerID + y][x] = data[line + x];
      }
    }
    line += linewidth;
  }
  
  free(bmp);
  return 0;
}

void findRegions(){
  u32 x,y;
  memset(rgnFinder, -1, sizeof(rgnFinder));
  rgnCount = 0;
  for(y = 0; y < BUFFER_HEIGHT; y++){
    for(x = 0; x < LAYER_WIDTH; x++){
      if(src[y][x] != 0 && rgnFinder[y][x] == -1){
        
        if(rgnCount >= rgnMax){
          if(rgnMax == 0){
            rgnMax = 16;
          }else{
            rgnMax *= 2;
          }
          regions = realloc(regions, sizeof(region) * rgnMax);
        }
        
        regions[rgnCount].id = rgnCount;
        regions[rgnCount].x1 = x;
        regions[rgnCount].y1 = y;
        regions[rgnCount].x2 = x;
        regions[rgnCount].y2 = y;
        regions[rgnCount].nextDuplicate = -1;
        regions[rgnCount].parent = -1;
        
        rgnFill(&regions[rgnCount], x, y);
        regions[rgnCount].w = regions[rgnCount].x2 - regions[rgnCount].x1 + 1;
        regions[rgnCount].h = regions[rgnCount].y2 - regions[rgnCount].y1 + 1;
        getRgnHash(&regions[rgnCount]);
        rgnCount++;
      }
    }
  }
  
  printf("Number of regions: %d\n", rgnCount);
}

void rgnFill(region* r, u32 x, u32 y){
  u32 x2;
  
  // Extend from current pixel to the right
  for(x2 = x+1; x2 < LAYER_WIDTH && src[y][x2] != 0 && rgnFinder[y][x2] == -1; x2++){
    rgnFinder[y][x2] = r->id;
  }
  
  // Extend from current pixel to the left
  for( ; x < LAYER_WIDTH && src[y][x] != 0 && rgnFinder[y][x] == -1; x--){
    rgnFinder[y][x] = r->id;
  }
  
  // Update region bounds
  if((x2 - 1) > r->x2) r->x2 = x2 - 1;
  if((x + 1) < r->x1) r->x1 = x + 1;
  if(y < r->y1) r->y1 = y;
  if(y > r->y2) r->y2 = y;
  
  for( ; (s32)x <= (s32)x2; x++){ // do signed so that -1 < x2 instead of being maximum value
    if(x >= LAYER_WIDTH) continue; // < 0 or > width
    
    // row above, if not the top row and the pixel is both valid and not already part of a region
    if(y > 0 && src[y-1][x] != 0 && rgnFinder[y-1][x] == -1) rgnFill(r, x, y-1);
    
    // row below, if not the bottom row and the pixel is both valid and not already part of a region
    if((y+1) < BUFFER_HEIGHT && src[y+1][x] != 0 && rgnFinder[y+1][x] == -1) rgnFill(r, x, y+1);
  }
}

void getRgnHash(region* r){
  u32 x,y;
  r->hash = 0xFFFFFFFFL;
  for(y = r->y1; y <= r->y2; y++){
    for(x = r->x1; x <= r->x2; x++){
      if(rgnFinder[y][x] == r->id){
        r->hash = crcNext(r->hash, src[y][x]);
      }else{
        r->hash = crcNext(r->hash, 0);
      }
    }
  }
  r->hash ^= 0xFFFFFFFFL;
}

void findDuplicates(){
  u32 i,j,x,y;
  bool same, a,b;
  u32 prev;
  
  for(i = 0; i < rgnCount; i++){
    if(regions[i].parent != -1){
      continue; // already processed
    }
    
    prev = i;
    for(j = i+1; j < rgnCount; j++){
      if(regions[j].parent != -1){
        continue; // already processed
      }
      
      // Quick checks...
      if(regions[j].hash != regions[i].hash || regions[j].w != regions[i].w || regions[j].h != regions[i].h){
        continue; // def not the same image
      }
      
      // Check byte-by-byte
      same = true;
      for(y = 0; same && y < regions[i].h; y++){
        for(x = 0; same && x < regions[i].w; x++){
          a = rgnFinder[regions[i].y1 + y][regions[i].x1 + x] == regions[i].id;
          b = rgnFinder[regions[j].y1 + y][regions[j].x1 + x] == regions[j].id;
          if(a != b || (a && b && src[regions[i].y1 + y][regions[i].x1 + x] != src[regions[j].y1 + y][regions[j].x1 + x])){
            same = false; // different shape or different pixels
          }
        }
      }
      
      if(same == false){
        continue; // not the same image
      }
      
      regions[j].parent = i; // parent = 1st instance of this image
      regions[prev].nextDuplicate = j; // keep a chain of the duplicate regions
      prev = j;
    }
  }
}

void parseRegions(){
  u32 i, count, size;
  regionStars = malloc(sizeof(starGroup*) * rgnCount);
  
  for(i = 0; i < rgnCount; i++){
    if(regions[i].parent != -1){
      regionStars[i] = NULL;
      continue;
    }
    count = (regions[i].w / MAX_STAR_DIM + 1) * (regions[i].h / MAX_STAR_DIM + 1);
    size = sizeof(starGroup) + sizeof(tmpStar) * (count - 1);
    regionStars[i] = malloc(size);
    memset(regionStars[i], 0, size);
    regionStars[i]->max = count;
    
    findSubImages(&regions[i], regionStars[i]);
  }
}

void findSubImages(region* r, starGroup* s){
  u32 x,y, x2,y2, w,h;
  s32 row, col;
  tmpStar* cur = NULL;
  
  // Find sub-images
  for(y = 0; y < r->h; y += MAX_STAR_DIM){
    for(x = 0; x < r->w; x += MAX_STAR_DIM){
      row = -1;
      col = -1;
      
      // find the first column with a pixel
      for(x2 = 0; col == -1 && x2 < MAX_STAR_DIM && (x + x2) < r->w; x2++){
        for(y2 = 0; y2 < MAX_STAR_DIM && (y + y2) < r->h; y2++){
          if(rgnFinder[r->y1 + y + y2][r->x1 + x + x2] == r->id){
            col = x2;
            break;
          }
        }
      }
      
      if(col == -1) continue; // There was nothing in the entirety of this subimage -- do not proceed
      
      // find the first row with a pixel
      for(y2 = 0; row == -1 && y2 < MAX_STAR_DIM && (y + y2) < r->h; y2++){
        for(x2 = 0; x2 < MAX_STAR_DIM && (x + x2) < r->w; x2++){
          if(rgnFinder[r->y1 + y + y2][r->x1 + x + x2] == r->id){
            row = y2;
            break;
          }
        }
      }
      
      // Found some data -- add it as a sub-star
      w = 0;
      h = 0;
      cur = &s->stars[s->count];
      cur->x = x + col;
      cur->y = y + row;
      // Copy image data, and clear the copied pixels to prevent redundancy
      for(y2 = 0; y2 < MAX_STAR_DIM && (y + y2 + row) < r->h; y2++){
        for(x2 = 0; x2 < MAX_STAR_DIM && (x + x2 + col) < r->w; x2++){
          if(rgnFinder[r->y1 + y + y2 + row][r->x1 + x + x2 + col] == r->id){
            cur->data[y2][x2] = src[r->y1 + y + y2 + row][r->x1 + x + x2 + col];
            rgnFinder[r->y1 + y + y2 + row][r->x1 + x + x2 + col] |= 0xC0000000L;
            if(x2 >= w) w = x2 + 1;
            if(y2 >= h) h = y2 + 1;
          }else{
            cur->data[y2][x2] = 0;
          }
        }
      }
      
      cur->w = w;
      cur->h = h;
      
      s->count++;
      if(s->count > s->max) printf("Something bad probably happened LMAO %d > %d\n", s->count, s->max);
    }
  }
}

void decompile(){
  u32 i,j, x,y, y1,x1;
  u32 fnum;
  bool genNames = false;
  u8 tmpname[260];
  
  u8* data;
  u32 size;
  starData* star;
  u32 starOffs;
  
  bmp bmp = {0x4D42, // "BM"
             sizeof(bmp) - 1 + ((LAYER_WIDTH+3)&(~3)) * LAYER_HEIGHT, // filesize
             0, // unused
             sizeof(bmp) - 1, // dataoffs
             { sizeof(BMI) - sizeof(pal), // size
               LAYER_WIDTH, // width
               LAYER_HEIGHT, // height
               1, // planes
               8, // bpp
               0, // compression
               ((LAYER_WIDTH+3)&(~3)) * LAYER_HEIGHT, // imgSize
               0, // hres
               0, // vres
               0, // palSize
               0, // icolors
               {0} // pal
             },
             {0} // data[0]
            };
  
  // Load Palette
  FILE * pFile = fopen("platform.wpe", "rb");
  if(pFile == NULL){
    puts("Error reading platform.wpe -- could not load palette.");
    return;
  }
  i = fread(pal, sizeof(RGBQUAD), 256, pFile);
  if(i != 256){
    puts("Error reading platform.wpe -- invalid format.");
    fclose(pFile);
    return;
  }
  fclose(pFile);
  for(i = 0; i < 256; i++){
    bmp.bmi.pal[i].rgbBlue = pal[i].rgbRed;
    bmp.bmi.pal[i].rgbGreen = pal[i].rgbGreen;
    bmp.bmi.pal[i].rgbRed = pal[i].rgbBlue;
  }
  
  // Load SPK
  printf("Reading %s ...\n", output);
  pFile = fopen(output, "rb");
  if(pFile == NULL){
    puts("Error loading SPK file.\n");
    return;
  }
  fseek(pFile, 0, SEEK_END);
  size = ftell(pFile);
  if(size == 0 || size == 0xFFFFFFFF){
    puts("ERROR: Could not read file.");
    fclose(pFile);
    return;
  }
  rewind(pFile);
  data = malloc(size);
  if(data == NULL){
    puts("ERROR: Could not allocate memory.");
    fclose(pFile);
    return;
  }
  fread(data, 1, size, pFile);
  fclose(pFile);
  
  layerCount = *(u16*)(data);
  printf("File contains %d layers.\n\n", layerCount);
  if(layerCount == 0){
    free(data);
    return;
  }
  
  stars = (starRef*)(data + (layerCount+1)*2);
  
  // Generate output filenames
  j = 0;
  if(inputCount == 0){ // No output filenames -- use default
    strcpy(input[0], output);
    fnum = strlen(output) - 4; // position of ".spk"
    if(layerCount == 1){
      strcpy(&input[0][fnum], "_out.bmp"); // change extension
    }else{
      input[0][fnum] = 0;
      sprintf(tmpname, "%s%%d.bmp", input[0]);  // add trailing digit & file extension
      genNames = true;
    }
  }else if(inputCount < layerCount && inputCount < MAX_LAYER_COUNT){ // not enough inputs were provided
    strcpy(tmpname, input[inputCount-1]);
    for(fnum = strlen(tmpname)-1; fnum > 0 && tmpname[fnum] != '.' && tmpname[fnum] != '\\'; fnum--);
    if(tmpname[fnum] == '.'){
      if(tmpname[fnum-1] >= '0' && tmpname[fnum-1] <= '9'){ // use existing trailing digit
        fnum--;
        j = tmpname[fnum] - inputCount;
      }
      tmpname[fnum-1] = 0;
    }
    strcat(tmpname, "%d.bmp");
    genNames = true;
  }
  if(genNames){
    for(i = inputCount; i < MAX_LAYER_COUNT; i++){
      sprintf(input[i], tmpname, j+i);
    }
  }
  
  if(layerCount <= MAX_LAYER_COUNT){
    memcpy(starCounts, &data[2], layerCount*sizeof(u16));
  }else{
    printf("WARNING: Layer contains more than the maximum number supported (%d).\nExtra layers will be ignored.\n", MAX_LAYER_COUNT);
    memcpy(starCounts, &data[2], MAX_LAYER_COUNT*sizeof(u16));
  }
  
  // Render the layers
  puts("\nRendering ...");
  starOffs = 0;
  for(i = layerCount; i > 0; ){ // stars get added in reverse layer order
    if(i > MAX_LAYER_COUNT){
      starOffs += ((u16*)(data))[i];
      continue;
    }
    i--; // spare all the [i-1]'s
    offsets[i] = starOffs;
    starOffs += starCounts[i];
    printf("  Layer %d -> %d stars\n", i, starCounts[i]);
    for(j = 0; j < starCounts[i]; j++){
      star = (starData*)(data + stars[offsets[i] + j].imgOffs);
      y1 = LAYER_OFFSET*i + stars[offsets[i] + j].y;
      x1 = stars[offsets[i] + j].x;
      for(y = 0; y < star->h && y < LAYER_HEIGHT; y++){
        for(x = 0; x < star->w && x < LAYER_WIDTH; x++){
          if(src[y1+y][x1+x] == 0){ // it draws this way, so render with it too
            src[y1+y][x1+x] = star->data[y*star->w + x];
          }
        }
      }
    }
  }
  
  // Save files
  puts("\nSaving ...");
  for(i = 0; i < layerCount; i++){
    printf("  Saving Layer %d to %s ... ", i, input[i]);
    pFile = fopen(input[i], "wb");
    if(pFile == NULL){
      puts("ERROR");
      continue;
    }
    fwrite(&bmp, 1, sizeof(bmp)-1, pFile);
    for(y = 0; y < LAYER_HEIGHT; y++){
      fwrite(src[LAYER_OFFSET*i + LAYER_HEIGHT - y - 1], 1, LAYER_WIDTH, pFile);
    }
    fclose(pFile);
    puts("Done");
  }
  
  free(data);
  puts("\nDone.");
}
