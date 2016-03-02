#ifndef CHIP8SFML_H
#define CHIP8SFML_H

#include <vector>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>

#include <SFML/Graphics.hpp>
#include "chip8cpu.h"


using namespace std;


class Chip8SFML{

public :
	Chip8SFML(); // constructor
	~Chip8SFML(); // destructor
	int loadRom(string filename); // load a rom file
	void draw(); // update screen
	void loop(); // main loop
	void start(); // start emulation
	void wait(); // wait for emulation to end

private :
	Chip8cpu         _cpu; // CPU
	sf::RenderWindow *_window; // screen
	thread           *_cpuThread; // make sure cpu runs at 60Hz
	chrono::system_clock::time_point _timeNext; // point in time where the next loop should happen
	bool             _loop; // tell wether the main loop is still alive
	
	

};


#endif // CHIP8SFML_H 
