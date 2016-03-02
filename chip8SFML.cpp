#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <SFML/Graphics.hpp>


#include "chip8SFML.h"
#include "chip8cpu.h"

using namespace std;



//constructor
Chip8SFML::Chip8SFML(): _cpu(Chip8cpu()),
                        _window(NULL),
						_cpuThread(NULL),
                        _timeNext(chrono::system_clock::now()),
                        _loop(false)
{
	_window = new sf::RenderWindow(sf::VideoMode(640, 320), "Chip8 Emulator");
	_window->setActive(false);
}


// destructor
Chip8SFML::~Chip8SFML()
{
	if (_cpuThread != NULL) 
	{
		delete _cpuThread;
	}
}


// load a rom file
int Chip8SFML::loadRom(string filename)
{
	return _cpu.loadRom(filename);
}
	
// update screen
void Chip8SFML::draw()
{
	sf::RectangleShape rectangle(sf::Vector2f(10, 10));
	rectangle.setFillColor(sf::Color(255, 255, 255));
	// clear the window with black color
	_window->clear(sf::Color::Black);
	for (int i=0; i<_cpu._S_WIDTH*_cpu._S_HEIGHT; i++){
		if (_cpu.getPix(i)) {
			int x = 10*(i % _cpu._S_WIDTH);
			int y = 10*(i / _cpu._S_WIDTH);
			rectangle.setPosition(x, y);
			_window->draw(rectangle);
		}
	}
	_window->display();

}

// main loop
void Chip8SFML::loop()
{
	// count the number of loop per second
	chrono::system_clock::time_point t0 = chrono::system_clock::now();
	uint cpt = 0;
	
	// infinite loop
	while(_loop)
	{
		_cpu.step();
		draw();		
		// set the point in time for the begining of next loop
		_timeNext += std::chrono::nanoseconds(16666667);
		this_thread::sleep_until( _timeNext );

		// display the number of loop during the last second
		cpt++;
		chrono::system_clock::time_point t1 = chrono::system_clock::now();
		if ((t1-t0) > std::chrono::seconds(1))
		{
			cout << cpt << "/sec" << endl;
			t0 = t1;
			cpt = 0;
		}
	}
}



// start emulation
void Chip8SFML::start()
{
	srand(time(0)); // use current time as seed for random generator
	_loop = true;
	_timeNext = chrono::system_clock::now();
	_cpuThread = new thread(&Chip8SFML::loop, this);
}

void Chip8SFML::wait()
{
	_cpuThread->join();
}
