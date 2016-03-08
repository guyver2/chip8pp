#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>


#include "chip8SFML.h"
#include "chip8cpu.h"

using namespace std;



//constructor
Chip8SFML::Chip8SFML(): _cpu(Chip8cpu()),
                        _window(NULL),
						_cpuThread(NULL),
                        _timeNext(chrono::system_clock::now()),
                        _loop(false),
                        _keyboard(vector<key>())
{
	_window = new sf::RenderWindow(sf::VideoMode(640, 320), "Chip8 Emulator");
	_window->setActive(false);
	initKeyboard();
}


// destructor
Chip8SFML::~Chip8SFML()
{
	if (_cpuThread != NULL) 
	{
		delete _cpuThread;
	}
	if (_window != NULL) 
	{
		delete _window;
	}
}

// init keyboard mapping 
void Chip8SFML::initKeyboard()
{
	_keyboard.push_back({false, sf::Keyboard::Numpad0});
	_keyboard.push_back({false, sf::Keyboard::Numpad7});
	_keyboard.push_back({false, sf::Keyboard::Numpad8});
	_keyboard.push_back({false, sf::Keyboard::Numpad9});
	_keyboard.push_back({false, sf::Keyboard::Numpad4});
	_keyboard.push_back({false, sf::Keyboard::Numpad5});
	_keyboard.push_back({false, sf::Keyboard::Numpad6});
	_keyboard.push_back({false, sf::Keyboard::Numpad1});
	_keyboard.push_back({false, sf::Keyboard::Numpad2});
	_keyboard.push_back({false, sf::Keyboard::Numpad3});
	_keyboard.push_back({false, sf::Keyboard::A});
	_keyboard.push_back({false, sf::Keyboard::S});
	_keyboard.push_back({false, sf::Keyboard::D});
	_keyboard.push_back({false, sf::Keyboard::Z});
	_keyboard.push_back({false, sf::Keyboard::X});
	_keyboard.push_back({false, sf::Keyboard::C});
}


// update keyboard and send values to the cpu
void Chip8SFML::updateKeyboard()
{
	for(int i=0; i < _keyboard.size(); i++){
		_keyboard[i].pressed = sf::Keyboard::isKeyPressed(_keyboard[i].code);
		_cpu.setKey(i, _keyboard[i].pressed);
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

// capture closing event
void Chip8SFML::processCloseEvent()
{
	sf::Event event;
	while (_window->pollEvent(event))
	{
		switch (event.type)
		{
		    // closed window
		    case sf::Event::Closed:
		        _loop = false;
		        break;
		    // escape pressed
		    case sf::Event::KeyPressed:
		        if (event.key.code == sf::Keyboard::Escape){
		        	_loop = false;
		        }    
		        break;
		    // everything else is done in updateKeyboard
		    default:
		        break;
		}
	}
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
		processCloseEvent();
		updateKeyboard();
		_loop = (_loop && !_cpu.step());
		draw();		
		// set the point in time for the begining of next loop
		_timeNext += std::chrono::nanoseconds((int)(1e9/_cpu._IPS));
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
	_window->close();
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
