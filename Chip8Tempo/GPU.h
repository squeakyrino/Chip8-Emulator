#include <iostream>

class GPU {

private:

	/*The graphics of the Chip 8 are black and white and the screen has a total of 2048 pixels(64 x 32).
	This can easily be implemented using an array that hold the pixel state(1 or 0).
	*/

	unsigned char gfx[64 * 32];
	

public:

	bool drawFlag;

	GPU::GPU() {

		for (size_t i = 0; i < 64 * 32; i++)
		{
				gfx[i] = 0;
		}

		drawFlag = false;
	}
	void draw() {

		if (drawFlag) {
			for (size_t i = 0; i < 32; i++)
			{
				for (size_t j = 0; j < 64; j++)
				{
					printf("%s", gfx[j + i * 64] != 0 ? "█" : " ");
				}
				printf("%d\n", i);
			}
		}
	}

	void clearScreen() {
		for (size_t i = 0; i < 64 * 32; i++)
		{
			gfx[i] = 0;
		}
	}

	unsigned char *getMem() {

		return gfx;
	}
};
