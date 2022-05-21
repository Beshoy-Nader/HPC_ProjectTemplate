#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header
#pragma once
#include<mpi.h>
#include<stdio.h>
#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;



int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;



	int OriginalImageWidth, OriginalImageHeight;



	//*********************************************************Read Image and save it to local arrayss*************************
	//Read Image and save it to local arrayss



	System::Drawing::Bitmap BM(imagePath);



	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int* Red = new int[BM.Height * BM.Width];
	int* Green = new int[BM.Height * BM.Width];
	int* Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height * BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);



			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;



			input[i * BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values



		}



	}
	return input;
}



void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);




	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i * width + j] < 0)
			{
				image[i * width + j] = 0;
			}
			if (image[i * width + j] > 255)
			{
				image[i * width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j], image[i * MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}
int main()
{
	MPI_Init(NULL, NULL);
	int rank, size ;
	int start_s, stop_s, TotalTime = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (rank == 0)
	{
		start_s = clock();
	}
	int ImageWidth = 4, ImageHeight = 4;
	System::String^ imagePath;
	std::string img;
	img = "..//Data//Input//in000001.jpg";
	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	free(imageData);

	int counter = 0;
	int* forGrdMsk = new int[ImageWidth * ImageHeight];
	int* pixelSum = new int[ImageWidth * ImageHeight];
	int* sumPixelSum = new int[ImageWidth * ImageHeight];
	int** bRankN = new int* [ImageWidth * ImageHeight];
	
	int sum = 0;
	for (size_t i = 0; i < ImageHeight * ImageWidth; i++)
	{
		bRankN[i] = new int[495 / size];
	}
	for (int i = (rank * 495 / size) + 1; i < (495 * (rank + 1) / size) + 1; i++)
	{
		
		System::String^ imagePath;
		std::string img;
		
		if (i < 10) {
			img = "..//Data//Input//in00000" + to_string(i) + ".jpg";
		}
		else if (i < 100)
		{
			img = "..//Data//Input//in0000" + to_string(i) + ".jpg";
		}
		else
		{
			img = "..//Data//Input//in000" + to_string(i) + ".jpg";
		}
		imagePath = marshal_as<System::String^>(img);
		int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
		for (int j = 0; j < ImageWidth * ImageHeight; j++)
		{
			bRankN[j][counter] = imageData[j];
		}
		counter++;
		//start_s = clock();
		//createImage(imageData, ImageWidth, ImageHeight, i);
		//stop_s = clock();
		//TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		//cout << "time: " << TotalTime << endl;
		free(imageData);
	}
	for (int k = 0; k < ImageHeight * ImageWidth; k++)
	{
		sum = 0;
		for (int j = 0; j < 495 / size; j++)
		{
			sum += bRankN[k][j];
		}
		pixelSum[k] = sum / (495 / size);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Reduce(pixelSum, sumPixelSum, ImageWidth * ImageHeight, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		for (int i = 0; i < ImageWidth * ImageHeight; i++)
		{
			sumPixelSum[i] = sumPixelSum[i] / size;
		}
		 createImage(sumPixelSum, ImageWidth, ImageHeight, 0);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(sumPixelSum, ImageWidth * ImageHeight, MPI_INT, 0, MPI_COMM_WORLD);
	counter = 0;
	int th = 50;
	for (int i = (rank * 495 / size) + 1; i < (495 * (rank + 1) / size) + 1; i++)
	{
		for (int j = 0; j < ImageWidth * ImageHeight; j++)
		{
			if (abs(bRankN[j][counter] - sumPixelSum[j]) > th)
			{
				forGrdMsk[j] = 255;
			}
			else
			{
				forGrdMsk[j] = 0;
			}
		}
		counter++;
		createImage(forGrdMsk, ImageWidth, ImageHeight, i);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
	{
		stop_s = clock();
		TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;
		cout << "time: " << TotalTime << endl;
	}
	MPI_Finalize();
	return 0;
}