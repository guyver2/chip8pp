#include <iostream>
#include "chip8SFML.h"


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
	
	Chip8SFML chip = Chip8SFML();
	cout << "rom size: " << chip.loadRom(romFilename) << endl;
	chip.start();
	chip.wait();
	return 0;
}
