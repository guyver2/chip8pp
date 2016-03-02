#include <iostream>
#include "chip8cpu.h"


using namespace std;

int main(int argc, char** argv)
{
	string romFilename;
	if (argc < 2)
	{
		romFilename = string("roms/MAZE");
	}
	else 
	{
		romFilename = string(argv[1]);
	}
	
	Chip8cpu cpu = Chip8cpu();
	cout << "rom size: " << cpu.loadRom(romFilename) << endl;
	cpu.start();
	cpu.wait();
	return 0;
}
