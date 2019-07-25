#include "stdafx.h"
#include "Memory.h"
#include "CPU.h"
#include "GPU.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <SFML\Graphics.hpp>

bool running = true;
void processKeyboard(short *keys);

bool loadRom(Memory* memory) {

	std::ifstream rom("../ROMS/spaceinvaders", std::ios::binary | std::ios::ate);

	if (rom.is_open()) {

		printf("Loaded\n");

		rom.seekg(0, rom.end);
		std::streamoff length = rom.tellg();
		rom.seekg(0, rom.beg);

		//char* memory2 = (char*)memory->getMem();
		//rom.read(&memory2[0x200], length);
		rom.read(&((char*)(memory->getMem()))[0x200], length);
		rom.close();
		return true;
	}
	else {

		//TODO logging?
		printf("Not Loaded\n");
		return false;
	}
}

void emulateCycle(CPU *cpu, Memory *memory, GPU *gpu, short *keys) {

	short opcode = memory->getMem()[cpu->getPC()] << 8 | memory->getMem()[cpu->getPC() + 1];

	int vx = (opcode & 0x0F00) >> 8;
	int vy = (opcode & 0x00F0) >> 4;
	switch (opcode & 0xF000) {

	case(0x0000):

		switch (opcode & 0x0000F) {

			/*
			00E0: Clears the screen.
			*/
		case(0x0000):
			gpu->drawFlag = true;
			gpu->clearScreen();
			cpu->setPC(cpu->getPC() + 2);
			return;

			/*
			00EE: Returns from a subroutine.
			*/
		case(0x000E):
			cpu->setPC(cpu->pop() + 2); //Pop the PC in the stack and add it 2 shorts
			return;
		}

		/*
		1NNN: Jumps to address NNN.
		*/
		case (0x1000):
		cpu->setPC(opcode & 0x0FFF);
		break;

		/*
		2NNN: Calls subroutine at NNN.
		*/
		case (0x2000):
		cpu->push();
		cpu->setPC(opcode & 0x0FFF);
		break;

		/*
		3XNN: Skips the next instruction if VX equals NN.
		(Usually the next instruction is a jump to skip a code block)
		*/
	case (0x3000):
		if (cpu->getRegister(vx) == (opcode & 0x00FF)) {
			cpu->setPC(cpu->getPC() + 4);
		}
		else {
			cpu->setPC(cpu->getPC() + 2);
		}
		break;

		/*
		4XNN: Skips the next instruction if VX doesn't equal NN.
		(Usually the next instruction is a jump to skip a code block)
		*/
	case (0x4000):
		if (cpu->getRegister(vx) != (opcode & 0x00FF)) {
			cpu->setPC(cpu->getPC() + 4);
		}
		else {
			cpu->setPC(cpu->getPC() + 2);
		}
		break;

		/*
		5XY0: Skips the next instruction if VX equals VY.
		(Usually the next instruction is a jump to skip a code block)
		*/
	case (0x5000):
		if (cpu->getRegister(vx) == cpu->getRegister(vy)) {
			cpu->setPC(cpu->getPC() + 4);
		}
		else {
			cpu->setPC(cpu->getPC() + 2);
		}
		break;

		/*
		6XNN: Sets VX to NN.
		*/
		case (0x6000):
		cpu->setRegister(vx, opcode & 0x00FF);
		cpu->setPC(cpu->getPC() + 2);
		break;

		/*
		7XNN: Adds NN to VX. (Carry flag is not changed)
		*/
	case (0x7000):
		cpu->setRegister(vx, (opcode & 0x00FF) + cpu->getRegister(vx));
		cpu->setPC(cpu->getPC() + 2);
		break;

	case (0x8000):

		switch (opcode & 0x000F) {

			/*
			8XY0: Sets VX to the value of VY.
			*/
		case (0x0000):
			cpu->setRegister(vx, cpu->getRegister(vy));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY1: Sets VX to VX or VY. (Bitwise OR operation)
			*/
		case (0x0001):
			cpu->setRegister(vx,
				(cpu->getRegister(vx)) | cpu->getRegister(vy));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY2: Sets VX to VX and VY. (Bitwise AND operation)
			*/
		case (0x0002):
			cpu->setRegister(vx,
				(cpu->getRegister(vx)) & cpu->getRegister(vy));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY3: Sets VX to VX xor VY.
			*/
		case (0x0003):
			cpu->setRegister(vx,
				(cpu->getRegister(vx)) ^ cpu->getRegister(vy));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
			*/
		case (0x0004):
			cpu->setRegister(vx, cpu->getRegister(vx) + cpu->getRegister(vy));
			if (cpu->getRegister(vy) > (0xFF - cpu->getRegister(vx))) {
				cpu->setRegister(0xF, 1); //V16 is the carry flag and the register too
			}
			else {
				cpu->setRegister(0xF, 0);
			}
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
			*/
		case(0x0005):
			if ((cpu->getRegister(vy) > cpu->getRegister(vx))) {
				/*
				To check if there's a borrow, check if b > a in a - b
				*/
				cpu->setRegister(0xF, 0); //There is a borrow
			}
			else {
				cpu->setRegister(0xF, 1);
			}
			cpu->setRegister(vx, cpu->getRegister(vx) - cpu->getRegister(vy));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY6: Shifts VY right by one and copies the result to VX. VF is set to the value of the least significant bit of VY before the shift.
			*/
			//TODO: Use VX instead?
		case(0x0006):
			/*
			cpu->setRegister(0xF, cpu->getRegister(vy) & 0x1);
			cpu->setRegister(vy, cpu->getRegister(vy) >> 1);
			cpu->setRegister(vx, cpu->getRegister(vy));
			*/
			cpu->setRegister(0xF, cpu->getRegister(vx) & 0x1);
			cpu->setRegister(vx, cpu->getRegister(vx) >> 1);
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
			*/
		case(0x0007):
			if (cpu->getRegister(vx) > cpu->getRegister(vy)) {
				/*
				To check if there's a borrow, check if b > a in a - b
				*/
				cpu->setRegister(0xF, 0); //There is a borrow
			}
			else {
				cpu->setRegister(0xF, 1);
			}
			cpu->setRegister(vx, cpu->getRegister(vy) - cpu->getRegister(vx));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			8XYE: Shifts VY left by one and copies the result to VX. VF is set to the value of the most significant bit of VY before the shift.
			*/
			//TODO: Shift VX instead of VY
		case(0x000E):
			/*
			cpu->setRegister(0xF, cpu->getRegister(vy) >> 7);
			cpu->setRegister(vy, cpu->getRegister(vy) << 1);
			cpu->setRegister(vx, cpu->getRegister(vy));
			*/
			cpu->setRegister(0xF, cpu->getRegister(vx) >> 7);
			cpu->setRegister(vx, cpu->getRegister(vx) << 1);
			cpu->setPC(cpu->getPC() + 2);
			break;
		}
		break;

		/*
		9XY0: Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block).
		*/
	case(0x9000):
		if (cpu->getRegister(vx) != cpu->getRegister(vy)) {
			cpu->setPC(cpu->getPC() + 4);
		}
		else {
			cpu->setPC(cpu->getPC() + 2);
		}
		break;

		/*
		ANNN: Sets I to the address NNN.
		*/
	case (0xA000): cpu->setIndexReg(opcode & 0x0FFF);
		cpu->setPC(cpu->getPC() + 2);
		break;

		/*
		BNNN: Jumps to the address NNN plus V0.
		*/
	case(0xB000):
		cpu->setPC(cpu->getRegister(0x0) + (opcode & 0x0FFF));
		break;

		/*
		CXNN:Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
		*/
	case(0xC000):
		srand(time(NULL));
		cpu->setRegister(vx, (rand() % (0xFF + 1)) & (opcode & 0x00FF));
		cpu->setPC(cpu->getPC() + 2);
		break;

		/*
		DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
		Each row of 8 pixels is read as bit-coded starting from memory location I;
		I value doesn’t change after the execution of this instruction.
		As described above, VF is set to 1 if any screen pixels are flipped from set to unset
		when the sprite is drawn, and to 0 if that doesn’t happen.
		*/
	case(0xD000): {

		unsigned char xPos = cpu->getRegister(vx);
		unsigned char yPos = cpu->getRegister(vy);
		unsigned short height = opcode & 0x000F;
		unsigned short pixel; //This pixel is 8 bits long. Just like the row

		gpu->drawFlag = true;

		cpu->setRegister(0xF, 0); //Reset the VF flag

		for (int y = 0; y < height; y++)
		{
			pixel = memory->getMem()[cpu->getIndexReg() + y];
			for (int x = 0; x < 8; x++)
			{
				/*
				Loop through the bits in pixel and see if they are set to 1
				If they are set than they will be XOR'd onto the screen
				*/
				if (pixel & (0b10000000) >> x) {

					/*
					Check if the pixel on the screen is already set to 1
					If it is, then by performing a XOR the pixel will be unset
					Thus a collisin ocured and the VF flag is set to 1
					*/

					if (gpu->getMem()[(xPos + x + ((yPos + y) * 64))]) cpu->setRegister(0xF, 1);

					gpu->getMem()[(xPos + x + ((yPos + y) * 64))] ^= 1;
				}
			}
		}
		cpu->setPC(cpu->getPC() + 2);
		break;
	}
				  /*
				  Multiple opcodes starting with E
				  */

	case(0xE000):

		switch (opcode & 0x000F) {

			/*
			EX9E: Skips the next instruction if the key stored in VX is pressed. (Usually the next instruction is a jump to skip a code block).
			*/
		case(0x000E):
			if (keys[vx]) {
				cpu->setPC(cpu->getPC() + 4);
			}
			else {
				cpu->setPC(cpu->getPC() + 2);
			}
			break;

			/*
			EXA1: Skips the next instruction if the key stored in VX isn't pressed. (Usually the next instruction is a jump to skip a code block).
			*/
		case(0x0001):
			if (keys[vx]) {
				cpu->setPC(cpu->getPC() + 2);
			}
			else {
				cpu->setPC(cpu->getPC() + 4);
			}
			break;
		}
		break;
		/*
		Multiple opcodes starting with F
		*/

	case(0xF000):

		switch (opcode & 0x00FF) {

			/*
			FX07: Sets VX to the value of the delay timer.
			*/
		case(0x0007):
			cpu->setRegister(vx, cpu->getDelayTimer());
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX0A: A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next key event)
			*/
		case(0x000A):
		{
			bool keyNotPressed = true;
			while (keyNotPressed) {
				processKeyboard(keys);
				for (int i = 0; i < 16 && keyNotPressed; i++)
				{
					if (keys[i]) {
						cpu->setRegister(vx, i);
						keyNotPressed = false;
					}
				}
			}
			cpu->setPC(cpu->getPC() + 2);
			break;
		}
		/*
		FX15: Sets the delay timer to VX.
		*/
		case(0x0015):
			cpu->setDelayTimer(cpu->getRegister(vx));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX18: Sets the sound timer to VX.
			*/
		case(0x0018):
			cpu->setSoundTimer(cpu->getRegister(vx));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX1E: Adds VX to I.
			VF is set to 1 when there is a range overflow (I+VX>0xFFF), and to 0 when there isn't.
			*/
		case(0x001E):
			if (cpu->getIndexReg() + cpu->getRegister(vx) > 0xFFF) {
				cpu->setRegister(0xF, 1);
			}
			else {
				cpu->setRegister(0xF, 0);
			}
			cpu->setIndexReg(cpu->getIndexReg() + cpu->getRegister(vx));
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX29: Sets I to the location of the sprite for the character in VX.
			Characters 0-F (in hexadecimal) are represented by a 4x5 font.
			*/
		case(0x0029):
			cpu->setIndexReg(cpu->getRegister(vx) * 5);
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX33: Stores the binary-coded decimal representation of VX,
			with the most significant of three digits at the address in I,
			the middle digit at I plus 1, and the least significant digit at I plus 2.
			(In other words, take the decimal representation of VX,
			place the hundreds digit in memory at location in I,
			the tens digit at location I+1, and the ones digit at location I+2.)
			*/
		case(0x0033):
			memory->getMem()[cpu->getIndexReg()] = cpu->getRegister(vx) / 100;
			memory->getMem()[cpu->getIndexReg() + 1] = (cpu->getRegister(vx) / 10) % 10;
			memory->getMem()[cpu->getIndexReg() + 2] = (cpu->getRegister(vx) % 100) % 10;
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX55: Stores V0 to VX (including VX) in memory starting at address I. I is increased by 1 for each value written.
			*/
			//TODO: Stupid I register
		case(0x0055):
			for (int i = 0; i <= vx; i++) {
				memory->getMem()[cpu->getIndexReg() + i] = cpu->getRegister(i);
				//cpu->setIndexReg(cpu->getIndexReg() + 1);
			}
			cpu->setPC(cpu->getPC() + 2);
			break;

			/*
			FX65: Fills V0 to VX (including VX) with values from memory starting at address I. I is increased by 1 for each value written.
			*/
			//TODO: Stupid I register
		case(0x0065):
			for (int i = 0; i <= vx; i++) {
				cpu->setRegister(i, memory->getMem()[cpu->getIndexReg() + i]);
				//cpu->setIndexReg(cpu->getIndexReg() + 1);
			}
			cpu->setPC(cpu->getPC() + 2);
			break;
		}
		break;
	}

	

}

void processKeyboard(short *keys) {

	/*
	1	2	3	C |	1	2	3	4
	4	5	6	D | q	w	e	r
	7	8	9	E | a	s	d	f
	A	0	B	F | z	x	c	v
	*/
	keys[1] = GetAsyncKeyState(0x31) ? 1 : 0;
	keys[2] = GetAsyncKeyState(0x32) ? 1 : 0;
	keys[3] = GetAsyncKeyState(0x33) ? 1 : 0;
	keys[4] = GetAsyncKeyState(0x51) ? 1 : 0;
	keys[5] = GetAsyncKeyState(0x57) ? 1 : 0;
	keys[6] = GetAsyncKeyState(0x45) ? 1 : 0;
	keys[7] = GetAsyncKeyState(0x41) ? 1 : 0;
	keys[8] = GetAsyncKeyState(0x53) ? 1 : 0;
	keys[9] = GetAsyncKeyState(0x44) ? 1 : 0;
	keys[0] = GetAsyncKeyState(0x58) ? 1 : 0;
	keys[10] = GetAsyncKeyState(0x5A) ? 1 : 0;
	keys[11] = GetAsyncKeyState(0x43) ? 1 : 0;
	keys[12] = GetAsyncKeyState(0x34) ? 1 : 0;
	keys[13] = GetAsyncKeyState(0x52) ? 1 : 0; //D
	keys[14] = GetAsyncKeyState(0x46) ? 1 : 0; //E
	keys[15] = GetAsyncKeyState(0x56) ? 1 : 0; //F

}

int main()
{
	Memory memory;
	CPU cpu;
	GPU gpu;

	short keys[16] = {};

	sf::RectangleShape whitePixel(sf::Vector2f(10, 10));


	if (loadRom(&memory))
	{
		sf::RenderWindow window(sf::VideoMode(640, 320), "Chip8");
		window.setFramerateLimit(30);
		while (window.isOpen())
		{
			if (cpu.getDelayTimer()) cpu.setDelayTimer(cpu.getDelayTimer() - 1);
			if (cpu.getSoundTimer()) cpu.setSoundTimer(cpu.getSoundTimer() - 1);

			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
					window.close();
			}

			processKeyboard(keys);
			emulateCycle(&cpu, &memory, &gpu, keys);
			if (gpu.drawFlag) {
				window.clear(sf::Color::Black);


				
				unsigned char *gpuBuffer = gpu.getMem();

				for (size_t x = 0; x < 64; x++)
				{
					for (size_t y = 0; y < 32; y++)
					{
						//White pixel
						if (gpuBuffer[x + 64 * y])
						{
							whitePixel.setPosition(x * 10, y * 10);
							window.draw(whitePixel);
						}
						

					}

				}
				gpu.drawFlag = false;

				window.display();

				
			}
		}
	}

	return 0;
}
