/* bmp2matrix-2.c
 *
 * This code acquires 3 matrices for 3 channels (RGB)
 * of a BMP image.
 *
 * Last modified on February 28, 2021
 *
 * by Freeman Sun
*/

#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <unistd.h>

// Global Variables
uint32_t bmpInfo[3];
// bmpInfo[0] : Data adress
// bmpInfo[1] : Width
// bmpInfo[2] : Height


void bmpInfoFill(FILE* bmpPtr)
{
	// Write Data adress, width, and height into bmpInfo[]
	
	// Arguments:
	// bmpPtr -> BMP file stream, opened in "rb" mode
	
	// This function calls fscanf() 26 times.
	
	uint32_t dataAdress, width, height;
	uint32_t counter = 0, counter2;
	uint8_t Byte;

	while(counter < 10) {

		fscanf(bmpPtr, "%c", &Byte);
		counter++;
	} //Byte 9

	// Data Adress
	fscanf(bmpPtr, "%c", &Byte); //Byte 10
	dataAdress = Byte;
	fscanf(bmpPtr, "%c", &Byte); //Byte 11
	dataAdress += Byte*0x100;
	fscanf(bmpPtr, "%c", &Byte); //Byte 12
	dataAdress += Byte*0x10000;
	fscanf(bmpPtr, "%c", &Byte); //Byte 13
	dataAdress += Byte*0x1000000;

	bmpInfo[0] = dataAdress;

	counter = 14;
	while(counter <= 17) {

		fscanf(bmpPtr, "%c", &Byte);
		counter++;
	} //Byte 17

	//Width data
	fscanf(bmpPtr, "%c", &Byte); //Byte 18
	width = Byte;
	fscanf(bmpPtr, "%c", &Byte); //Byte 19
	width += Byte*0x100;
	fscanf(bmpPtr, "%c", &Byte); //Byte 20
	width += Byte*0x10000;
	fscanf(bmpPtr, "%c", &Byte); //Byte 21
	width += Byte*0x1000000;

	bmpInfo[1] = width;

	// Height data
	fscanf(bmpPtr, "%c", &Byte); //Byte 22
	height = Byte;
	fscanf(bmpPtr, "%c", &Byte); //Byte 23
	height += Byte*0x100;
	fscanf(bmpPtr, "%c", &Byte); //Byte 24
	height += Byte*0x10000;
	fscanf(bmpPtr, "%c", &Byte); //Byte 25
	height += Byte*0x1000000;

	bmpInfo[2] = height;
} // bmpInfoFill()

void pixels2pipe(FILE* bmpPtr, int pipe_w)
{
	// Transfer pixel data into the pipe
	
	// Arguments:
	// bmpPtr -> BMP file stream, opened in "rb" mode
	// pipe_w -> pipe for writing
	
	// Recommended to call after bmpInfoFill()
	
	int mod4 = (bmpInfo[1]%4);
	// In BMP file format, number of Bytes of
	// each row (horizontal pixels) must be a
	// multiple of 4
	
	int counter = 26;
	// After bmpInfoFill(), fscanf() will be at Byte 26
	
	uint8_t Byte;
	int counter2 = 0;
	int c3;
	
	// From Byte 26 to data adress byte
	while(counter < bmpInfo[0]) {

		fscanf(bmpPtr, "%c", &Byte);
		counter++;
	} // Data adress Byte
	
	printf("Data adress: %d\n", counter);
	
	counter = 0;
	while(counter < (bmpInfo[1]*bmpInfo[2])) {
		
		if(counter2 < bmpInfo[1]) {
			
			fscanf(bmpPtr, "%c", &Byte);
			write(pipe_w, &Byte, sizeof(Byte));
			fscanf(bmpPtr, "%c", &Byte);
			write(pipe_w, &Byte, sizeof(Byte));
			fscanf(bmpPtr, "%c", &Byte);
			write(pipe_w, &Byte, sizeof(Byte));
			counter++;
			
			counter2++;
		} else {
			
			for(c3 = 0; c3 < mod4; c3++) {
				
				fscanf(bmpPtr, "%c", &Byte);
			}
			
			counter2 = 0;
		}
	}
	
	printf("Pixel: %d\n", counter);
} // pixels2pipe()

void pipe2matrices(
	int pipe_r,
	uint8_t R_m[bmpInfo[2]][bmpInfo[1]],
	uint8_t G_m[bmpInfo[2]][bmpInfo[1]],
	uint8_t B_m[bmpInfo[2]][bmpInfo[1]])
{
	// Transfer the data from the pipe into the RGB matrices
	
	// Arguments:
	// pipe_r -> "read" end of the pipe
	// R_m -> RED Channel Matrix
	// G_m -> GREEN Channel Matrix
	// B_m -> BLUE Channel Matrix
	
	uint32_t x,y;
	uint8_t Byte;
	for(y = 1; y <= bmpInfo[2]; y++) {
		
		for(x = 0; x < bmpInfo[1]; x++) {
			
			read(pipe_r, &Byte, sizeof(uint8_t));
			B_m[bmpInfo[2]-y][x] = Byte;
			read(pipe_r, &Byte, sizeof(uint8_t));
			G_m[bmpInfo[2]-y][x] = Byte;
			read(pipe_r, &Byte, sizeof(uint8_t));
			R_m[bmpInfo[2]-y][x] = Byte;
		}
	}
} // pipe2matrices()

int main(int argc, char** argv)
{
	const char *bmpFileName = argv[1];
	
	FILE *fPtr;
	
	if( (fPtr = fopen(bmpFileName, "rb")) == NULL ) {

		MessageBox(0, argv[1], "Not opened!",
			MB_ICONEXCLAMATION | MB_OK | MB_SYSTEMMODAL);
		
		return -1;
	}
	
	// Write Data adress, width, and height into bmpInfo[]
	bmpInfoFill(fPtr);
	
	// bmpInfo[1] : Width
	// bmpInfo[2] : Height
	uint8_t R_matrix[bmpInfo[2]][bmpInfo[1]];
	uint8_t G_matrix[bmpInfo[2]][bmpInfo[1]];
	uint8_t B_matrix[bmpInfo[2]][bmpInfo[1]];
	
	// Create a pipe,
	int pipefd[2]; // File Decriptors for 2 ends of the pipe
	_pipe(pipefd, 0, 0);
	
	// Transfer pixel data into the pipe
	pixels2pipe(fPtr, pipefd[1]);
	
	fclose(fPtr); //close BMP file
	
	printf("width: %d , height: %d\n", bmpInfo[1], bmpInfo[2]);
	
	// Transfer the data from the pipe into the RGB matrices
	pipe2matrices(pipefd[0], R_matrix, G_matrix, B_matrix);
	
	uint32_t x,y;
	
	// Print RED channel matrix
	printf("\nRED\n");
	for(y = 0; y < bmpInfo[2]; y++) {
		
		for(x = 0; x < bmpInfo[1]; x++) {
			
			printf("%d\t", R_matrix[y][x]);
		}
		printf("\n");
	}
	
	// Print GREEN channel matrix
	printf("\nGREEN\n");
	for(y = 0; y < bmpInfo[2]; y++) {
		
		for(x = 0; x < bmpInfo[1]; x++) {
			
			printf("%d\t", G_matrix[y][x]);
		}
		printf("\n");
	}
	
	// Print BLUE channel matrix
	printf("\nBLUE\n");
	for(y = 0; y < bmpInfo[2]; y++) {
		
		for(x = 0; x < bmpInfo[1]; x++) {
			
			printf("%d\t", B_matrix[y][x]);
		}
		printf("\n");
	}
	
	return 0;
} // main()