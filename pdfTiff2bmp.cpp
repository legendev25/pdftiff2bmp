// pdfTiff2bmp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string>
#include <tiffio.h>
#include <iostream>
#include <algorithm>
#include <memory>
#include <Windows.h>

#include "mupdf/memento.h"
#include "mupdf/helpers/mu-office-lib.h"
#include "pdf.h"
#include "FreeImage.h"

using namespace std;

#define PBWIDTH 60

int		ConvertTiffToBmp(char* pszFilename, char* dstFolder, char* dstPrefix, int colorMode);
int		ConvertPdfToBmp(char* pszFilename, char* dstFolder, char* dstPrefix, int colorMode, float dpi);
void	ShowProgress(double progress);
int		ConvertAndSave(char* pszFilename, BYTE *pSource, int width, int height, int bpp, bool mirror);

void printfhelp()
{
	printf("Usage:\n");
	printf("pdftiff2bmp --help\n");
	printf("pdftiff2bmp source_type password source_file destination_folder output_name_prefix [-color BitPerPixel] [-dpi DotPerInch]\n");
	printf("\n");

	printf("Arguments:\n");
	printf("source_type: \"-pdf\" for PDF, \"-tiff\" for TIFF\n");
	printf("password: Password for private\n");
	printf("source_file: Full file path to extract\n");
	printf("destionation_folder: Folder path to save output bmp files\n");
	printf("output_name_prefix: Prefix string for output file name; Output file will have a name like PREFIX(page).bmp\n");
	printf("-colormode BitPerPixel: Specify BitPerPixel for output bmp file. Default value is 24bpp; 1, 2, 4, 8, 16, 24, 32; ex: -colormode 24\n");
	printf("-dpi DotPerInch: Specify DotPerInch for output bmp file. Ignored for tiff image. Default value is 200dpi; ex: -dpi 72\n");
}
int main(int argc, char* argv[])
{
	char *password, *sourceFile, *dstFolder, *dstPrefix;
	int colorMode, sourceType, dpi;

#ifndef _DEBUG
	if (argc == 1)
	{
		printfhelp();
		return 0;
	}
	else if (argc == 2)
	{
		if (!strcmp(argv[1], "--help"))
			printfhelp();
		else
		{
			printf("Invalid Arguments");
			return 0;
		}
	}
	if (argc == 6)
	{
		if (!strcmp(argv[1], "-pdf"))
		{
			dpi = 200;
			sourceType = 1;
		}
		else if (!strcmp(argv[1], "-tiff"))
			sourceType = 2;
		else
		{
			printf("Invalid Arguments\n");
			return 0;
		}

		password = argv[2];
		sourceFile = argv[3];
		dstFolder = argv[4];
		dstPrefix = argv[5];

		if (strcmp(password, "yuenyuen"))
		{
			printf("Password is not correct\n");
			return 0;
		}

		CreateDirectoryA(dstFolder, NULL);

		colorMode = 24;
	}
	else if (argc == 8)
	{
		if (!strcmp(argv[1], "-pdf"))
			sourceType = 1;
		else if (!strcmp(argv[1], "-tiff"))
			sourceType = 2;
		else
		{
			printf("Invalid Arguments\n");
			return 0;
		}

		password = argv[2];
		sourceFile = argv[3];
		dstFolder = argv[4];
		dstPrefix = argv[5];

		if (strcmp(password, "yuenyuen"))
		{
			printf("Password is not correct\n");
			return 0;
		}

		if (strcmp(argv[6], "-color") && strcmp(argv[6], "-dpi"))
		{
			printf("Invalid Arguments\n");
			return 0;
		}
		else if (!strcmp(argv[6], "-color"))
		{
			colorMode = atoi(argv[7]);
			dpi = 200;
		}
		else if (!strcmp(argv[6], "-dpi"))
		{
			dpi = atoi(argv[7]);
			colorMode = 24;
		}

		CreateDirectoryA(dstFolder, NULL);
	}
	else if(argc == 10)
	{
		if (!strcmp(argv[1], "-pdf"))
			sourceType = 1;
		else if (!strcmp(argv[1], "-tiff"))
			sourceType = 2;
		else
		{
			printf("Invalid Arguments\n");
			return 0;
		}

		password = argv[2];
		sourceFile = argv[3];
		dstFolder = argv[4];
		dstPrefix = argv[5];

		if (strcmp(password, "yuenyuen"))
		{
			printf("Password is not correct\n");
			return 0;
		}

		if (strcmp(argv[6], "-color"))
		{
			printf("Invalid Arguments\n");
			return 0;
		}

		if (strcmp(argv[8], "-dpi"))
		{
			printf("Invalid Arguments\n");
			return 0;
		}

		CreateDirectoryA(dstFolder, NULL);

		colorMode = atoi(argv[7]);
		dpi = atoi(argv[9]);
	}
	else
	{
		printf("Invalid arguments\n");
		return 0;
	}
#else
	sourceType = 1;
	dstFolder = "C:\\Users\\Legend\\Desktop\\output";
	dstPrefix = "sample";
	sourceFile = "1.pdf";
	colorMode = 4;
#endif

	std::cout << "Copyright (C) QSM SYSYEMS (HK) LTD. IMAGE CONVERTOR - Saving Image" << endl;

	DWORD startTime = GetTickCount();

	if (sourceType == 1)
		ConvertPdfToBmp(sourceFile, dstFolder, dstPrefix, colorMode, dpi);
	else
		ConvertTiffToBmp(sourceFile, dstFolder, dstPrefix, colorMode);

	DWORD endTime = GetTickCount();
	DWORD elapsedTime = (endTime - startTime) / 1000;

	printf("\n%d minutes %d seconds\n", elapsedTime / 60, elapsedTime % 60);

	return 0;
}

static HANDLE loaded;

int ConvertPdfToBmp(char* pszFilename, char* dstFolder, char* dstPrefix, int colorMode, float dpi)
{
	MuOfficeLib *mu;

	MuOfficeDoc *doc;
	MuError err;
	int count;
	MuOfficeRender *render;

	int ret;

	err = MuOfficeLib_create(&mu);
	if (err)
	{
		printf("mupdf: Failed to create lib instance: error=%d\n", err);
		return -1;
	}

	loaded = CreateSemaphore(NULL, 0, 1, NULL);

	err = MuOfficeLib_loadDocument(mu,
		pszFilename,
		NULL,
		NULL,
		NULL,
		&doc);

	/* Test number of pages */
	err = MuOfficeDoc_getNumPages(doc, &count);
	if (err)
	{
		printf("Failed to count pages: error=%d\n", err);
		return -1;
	}

	for (int i = 0; i < count; i++)
	{
		MuOfficePage *page;
		float w, h;
		MuOfficeBitmap bitmap;
		MuOfficeRenderArea area;

		/* Get a page */
		err = MuOfficeDoc_getPage(doc, i, NULL, (void *)4321, &page);
		if (err)
		{
			printf("Failed to get page: error=%d\n", err);
			return -1;
		}

		/* Get size of page */
		err = MuOfficePage_getSize(page, &w, &h);
		if (err)
		{
			printf("Failed to get page size: error=%d\n", err);
			return -1;
		}

		/* Allocate ourselves a bitmap */
		bitmap.width = (int)(w * (dpi / 90.0f) + 0.5f);
		bitmap.height = (int)(h * (dpi / 90.0f) + 0.5f);
		bitmap.lineSkip = bitmap.width * 4;
		bitmap.memptr = malloc(bitmap.lineSkip * bitmap.height);

		/* Set the area to render the whole bitmap */
		area.origin.x = 0;
		area.origin.y = 0;
		area.renderArea.x = 0;
		area.renderArea.y = 0;
		area.renderArea.width = bitmap.width;
		area.renderArea.height = bitmap.height;

		/* Render into the bitmap */
		err = MuOfficePage_render(page, dpi / 90.0f, &bitmap, &area, NULL, NULL, &render);
		if (err)
		{
			printf("Page render failed: error=%d\n", err);
			return -1;
		}

		err = MuOfficeRender_waitUntilComplete(render);
		if (err)
		{
			printf("Page render failed to complete: error=%d\n", err);
			return -1;
		}

		char dstName[1024];
		sprintf(dstName, "%s\\%s(%d).bmp", dstFolder, dstPrefix, i + 1);

		// Save the pixmap to a file.
		ConvertAndSave(dstName, (BYTE*)bitmap.memptr, bitmap.width, bitmap.height, colorMode, true);

		ShowProgress((double)(i + 1) / (double)(count) * 100.0);

		free(bitmap.memptr);
		MuOfficePage_destroy(page);
	}

	/* Kill the render */
	MuOfficeRender_destroy(render);
	MuOfficeDoc_destroy(doc);

	MuOfficeLib_destroy(mu);
}

int ConvertTiffToBmp(char* pszFilename, char* dstFolder, char* dstPrefix, int colorMode)
{
	int frameIndex = 0;
	uint16 pageCount = 0;

	// Open the TIFF file using libtiff
	TIFF* tif = TIFFOpen(pszFilename, "r");

	if (tif)
	{
		do{
			pageCount++;
		} while (TIFFReadDirectory(tif));
		TIFFClose(tif); // close the tif file
	}
	else
		return -1;

	tif = TIFFOpen(pszFilename, "r");

	// check the tif is open
	if (tif) {

		do {
			unsigned int width, height;

			uint32* raster;

			// get the size of the tiff
			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

			unsigned int npixels = width*height; // get the total number of pixels

			raster = (uint32*)_TIFFmalloc(npixels * sizeof(uint32)); // allocate temp memory (must use the tiff library malloc)
			if (raster == NULL) // check the raster's memory was allocaed
			{
				TIFFClose(tif);
				printf("Could not allocate memory for raster of TIFF image\n");
				return -1;
			}

			// Check the tif read to the raster correctly
			if (!TIFFReadRGBAImage(tif, width, height, raster, 0))
			{
				TIFFClose(tif);
				printf("Could not read raster of TIFF image\n");
				return -1;
			}

			frameIndex++;

			char bmpName[1024] = "";
			sprintf(bmpName, "%s\\%s(%d).bmp", dstFolder, dstPrefix, frameIndex);

			ConvertAndSave(bmpName, (BYTE*)raster, width, height, colorMode, false);

			ShowProgress((double)frameIndex / (double)pageCount * 100.0);

			_TIFFfree(raster);

		} while (TIFFReadDirectory(tif)); // get the next tif
		TIFFClose(tif); // close the tif file
	}
}

void ShowProgress( double progress )
{
	std::cout << "\r" << progress << "% completed: ";
	std::cout.flush();
}

int ConvertAndSave(char* pszFilename, BYTE *pSource, int width, int height, int bpp, bool mirror)
{
	FIBITMAP *fbitmapConverted = NULL;

	FIBITMAP *fbitmapOrigin = FreeImage_ConvertFromRawBits(pSource, width, height, width * 4, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, mirror);

	if (!fbitmapOrigin)
		return -1;

	switch (bpp)
	{
		case 1:
		{
			fbitmapConverted = FreeImage_Dither(fbitmapOrigin, FID_FS);
			break;
		}
		case 4:
		{
			fbitmapConverted = FreeImage_ConvertTo4Bits(fbitmapOrigin);
			break;
		}
		case 8:
		{
			fbitmapConverted = FreeImage_ConvertTo8Bits(fbitmapOrigin);
			break;
		}
		case 16:
		{
			fbitmapConverted = FreeImage_ConvertTo16Bits555(fbitmapOrigin);
			break;
		}
		case 24:
		{
			fbitmapConverted = FreeImage_ConvertTo24Bits(fbitmapOrigin);
			break;
		}
		case 32:
			break;
		default:
		{
			fbitmapConverted = FreeImage_ConvertTo24Bits(fbitmapOrigin);
		}
	}

	if (bpp == 32)
		FreeImage_Save(FIF_BMP, fbitmapOrigin, pszFilename);
	else
	{
		if (!fbitmapConverted)
		{
			FreeImage_Unload(fbitmapOrigin);
			return -1;
		}

		FreeImage_Save(FIF_BMP, fbitmapConverted, pszFilename);
		FreeImage_Unload(fbitmapConverted);
	}

	FreeImage_Unload(fbitmapOrigin);

	return 0;
}