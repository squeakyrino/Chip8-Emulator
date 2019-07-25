#include <stdlib.h>

class CPU {

private:

	//16 registers in the CHIP8 CPU
	unsigned char registers[16];

	//Index register
	unsigned short i;

	//Program counter
	unsigned short pc;

	/*
	Stack. Has 16 layers
	*/

	unsigned short stack[16];
	unsigned short sp;

	/*
	There are two timer registers that count at 60 Hz.
	When set above zero they will count down to zero.
	*/

	unsigned char delay_timer;
	unsigned char sound_timer;
public:

	CPU::CPU() {

		//Clear the registers

		for (size_t i = 0; i < 16; i++)
		{
			registers[i] = 0;

		}

		i = 0;

		pc = 0x200; //Start of the ROM programs

		for (size_t i = 0; i < 16; i++)
		{
			stack[i] = 0;
		}

		sp = 0;

		delay_timer = 0;
		sound_timer = 0;
	}

	unsigned char getRegister(int registerNum) {

		return registers[registerNum];
	}

	void setRegister(int registerNum, unsigned char value) {

		registers[registerNum] = value;
	}

	unsigned short getIndexReg() const {

		return i;
	}

	unsigned short getPC() const {

		return pc;
	}

	void setPC(unsigned short pc1) {
		pc = pc1;
	}

	void setIndexReg(unsigned short i1) {
		i = i1;
	}

	void push() {
		stack[sp] = pc;
		sp++;
	}

	unsigned short pop() {
		sp--;
		return stack[sp];
	}

	unsigned char getDelayTimer() {
		return delay_timer;
	}

	void setDelayTimer(unsigned char newTimer) {
		delay_timer = newTimer;
	}

	void setSoundTimer(unsigned char newTimer) {
		sound_timer = newTimer;
	}

	unsigned char getSoundTimer() {
		return sound_timer;
	}
};
