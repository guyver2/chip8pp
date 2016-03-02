#include "chip8cpu.h"

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <chrono>
#include <ctime>


using namespace std;

//constructor
Chip8cpu::Chip8cpu(): _mem(vector<uint8_t>(0xFFF, 0)),
                      _reg(vector<uint8_t>(16, 0)),
                      _I(0),
                      _regDelay(0),
                      _regSound(0),
                      _PC(0x200),
                      _SP(0),
                      _stack(vector<uint16_t>()),
                      _screen(vector<uint8_t>(_S_WIDTH*_S_HEIGHT, 0)),
                      _cpuThread(NULL),
                      _timeNext(chrono::system_clock::now()),
                      _loop(false),
                      _opCodeMasks(vector<opCodeMask>())
{
	initOpCodeMasks();
}


// destructor
Chip8cpu::~Chip8cpu()
{
	if (_cpuThread != NULL) 
	{
		delete _cpuThread;
	}
}

// load a rom from a buffer 
int Chip8cpu::loadRom(const vector<uint8_t> rom)
{
	std::copy ( rom.begin(), rom.end(), _mem.begin()+0x200 );
	return rom.size();
}


// load a rome from file
int Chip8cpu::loadRom(const string filename)
{
	ifstream file(filename,ios::in|ios::binary|ios::ate);
	if (file.is_open())
	{
		file.seekg(0, ios::end);
    	uint size = file.tellg();
    	file.seekg (0, ios::beg);
    	file.read ((char*)&(_mem[0x200]), size);
    	file.close();
		return size;
	}
	else
	{
		cerr << "Error while loading rom file : cannot open file. " << filename;
		cerr << endl;
		return 0;
	}
}


// main emulation loop
void Chip8cpu::cpuLoop()
{
	// count the number of loop per second
	chrono::system_clock::time_point t0 = chrono::system_clock::now();
	uint cpt = 0;
	
	// infinite loop
	while(_loop)
	{
		// updateKeyboard();
		uint16_t ins = readInstruction();
		int insID = decodeInstruction(ins);
		if (insID < 0) break;
		else
		{
			executeInstruction(insID, ins);
		}
		updateTimers();	
		//display();	
		_PC += 2;
		
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
void Chip8cpu::start()
{
	srand(time(0)); // use current time as seed for random generator
	_loop = true;
	_timeNext = chrono::system_clock::now();
	_cpuThread = new thread(&Chip8cpu::cpuLoop, this);
}

void Chip8cpu::wait()
{
	_cpuThread->join();
}

// update delay and sound timers, also play sound when needed
void Chip8cpu::updateTimers()
{
	if (_regDelay > 0) _regDelay--;
	if (_regSound > 0)
	{
		_regSound--;
		cout << "BEEP" << endl;
	}
}


// read current instruction
uint16_t Chip8cpu::readInstruction()
{
	return ((uint16_t)_mem[_PC]) << 8 | _mem[_PC+1];
}


// decode current instruction and return it's ID
int Chip8cpu::decodeInstruction(uint16_t ins)
{
	bool found = false;
	int i;
	for(i=0; i < _opCodeMasks.size(); i++)
	{
		if ((ins & _opCodeMasks[i].mask) == _opCodeMasks[i].val)
		{
			found = true;
			break ;
		}
	}
	if (found) 
	{
		cout << "instruction : " << i << " (" 
		     << std::showbase << std::internal << std::setfill('0')
		     << std::hex << std::setw(6) << ins << ")" << std::dec << endl;
		return i;
	}
	else
	{
		return -1;
	}
}

// execute an instruction based on it's ID
bool Chip8cpu::executeInstruction(int insID, uint16_t ins)
{
	uint8_t X = (ins & 0x0F00) >> 8;
	uint8_t Y = (ins & 0x00F0) >> 4;
	uint8_t N = ins & 0x000F;
	bool success = true;
	int tmp;
	switch(insID) 
	{
		case 0: // 0NNN
			cerr << "instruction 0NNN should not happen !" << endl;
			break;
		
		case 1: // 00E0 clear screen
			_screen.assign(64*32, 0);
			break;
		
		case 2: // 00EE return from subroutine
			_PC = _stack[_SP];
			_PC -= 2;
			_SP -= 1;
			if (_SP < 0){
				cerr << "ERROR : stack underflow" << endl;
				success = false;
			}
			break;
		
		case 3: // 1NNN jump to address NNN
			_PC = ((uint16_t)X<<8) | (Y<<4) | N;
			_PC -= 2;
			break;
		
		case 4: // 2NNN call subroutine
			_stack[_SP] = _PC;
			_SP += 1;
			_PC = ((uint16_t)X<<8) | (Y<<4) | N;
			if (_SP >= 16){
				cerr << "ERROR : stack overflow" << endl;
				success = false;
			}
			break;
			
		case 5: // 3XNN Skip next instruction if Vx = NN
			if (_reg[X] == ((Y<<4) | N) ){
				_PC += 2;
			}
			break;
		
		case 6: // 4XNN Skip next instruction if Vx != NN
			if (_reg[X] != ((Y<<4) | N) ){
				_PC += 2;
			}
			break;
		
		case 7: // 5xy0 Skip next instruction if Vx = Vy.
			if (_reg[X] == _reg[Y]) {
				_PC += 2;
			}
			break;
			
		case 8: // 6xNN Set Vx = NN
			_reg[X] = (Y<<4) | N;
			break;
			
		case 9: // 7xNN Vx = Vx + kk.
			_reg[X] += (Y<<4) | N;
			break;

		case 10: //	8xy0 Set Vx = Vy
			_reg[X] = _reg[Y];
			break;
		
		case 11: // 8xy1 Set Vx = Vx OR Vy
			_reg[X] = _reg[X] | _reg[Y];
			break;

		case 12: // 8xy2 Set Vx = Vx AND Vy
			_reg[X] = _reg[X] & _reg[Y];
			break;

		case 13: // 8xy3 Set Vx = Vx XOR Vy
			_reg[X] = _reg[X] ^ _reg[Y];
			break;

		case 14: // 8xy4 Set Vx = Vx + Vy, set VF = carry
			tmp = (uint16_t)_reg[X] + (uint16_t)_reg[Y];
			_reg[X] = tmp % 256;
			if (tmp < 255) _reg[15] = 1;
			else           _reg[15] = 0;
			break;
		
		case 15: // 8xy5 Set Vx = Vx - Vy, set VF = NOT borrow
			tmp = int(_reg[X]) - _reg[Y];
			if (tmp >= 0) _reg[15] = 1;
			else          _reg[15] = 0;
			_reg[X] = tmp % 256;
			break;
			
		case 16: // 8xy6 - Set Vx = Vx SHR 1.
			_reg[15] = _reg[X] & 0x01;
			_reg[X] >>= 1;
			break;
			
		case 17: // 8xy7 Set Vx = Vy - Vx, set VF = NOT borrow.
			tmp = int(_reg[Y]) - _reg[X];
			if (tmp >= 0) _reg[15] = 1;
			else          _reg[15] = 0;
			_reg[X] = tmp % 256;
			break;
		
		case 18: // 8xyE Set Vx = Vx SHL 1.
			_reg[15] = _reg[X]>>7;
			_reg[X]<<=1;
			break;
		
		case 19: // 9xy0 Skip next instruction if Vx != Vy
			if (_reg[X] != _reg[Y]) {
				_PC += 2;
			}
			break;
			
		case 20: // Annn Set I = nnn.
			_I = ((uint16_t)X<<8) | (Y<<4) | N;
			break;
		
		case 21: // Bnnn Jump to location nnn + V0
			_PC = (((uint16_t)X<<8) | (Y<<4) | N) + _reg[0];
			_PC -= 2;
			break;
		
		case 22: // CXNN Sets VX to a random number modulo NN+1
			_reg[X] = rand()%(((Y<<4) | N)+1 );
			break;
		
		case 23: // Dxyn Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
			{ // new block to allow local variables
				_reg[15] = 0; // no pixel flip so far
				for(int k=0; k<N; k++){
					uint8_t row = _mem[_I+k]; //get octet of the current line to draw
					int y = (_reg[Y]+k) % _S_HEIGHT; // y of affected pixel
					for (int j=0; j<8; j++)	{
						int x = (_reg[X]+j)%_S_WIDTH; // x of affected pixel
						if (row & (0x1<<(7-j))) {
							if (_screen[x+y*_S_WIDTH]) {
								_screen[x+y*_S_WIDTH] = 0;
								_reg[15] = 1; // flip happened
							} else {
								_screen[x+y*_S_WIDTH] = 1;
							}
						}
					}
				}
			}
			break;
			

			
		default :
			cout << "instruction " << insID << " not implemented yet" << endl;
	}
	
	return success;
}




void Chip8cpu::initOpCodeMasks(){
	_opCodeMasks.push_back({0xF000, 0x0000}); // 0NNN -- 0
	_opCodeMasks.push_back({0xFFFF, 0x00E0}); // 00E0
	_opCodeMasks.push_back({0xFFFF, 0x00EE}); // 00E0
	_opCodeMasks.push_back({0xF000, 0x1000}); // 1NNN
	_opCodeMasks.push_back({0xF000, 0x2000}); // 2NNN
	_opCodeMasks.push_back({0xF000, 0x3000}); // 3XNN -- 5
	_opCodeMasks.push_back({0xF000, 0x4000}); // 4XNN
	_opCodeMasks.push_back({0xF00F, 0x5000}); // 5XY0
	_opCodeMasks.push_back({0xF000, 0x6000}); // 6XNN
	_opCodeMasks.push_back({0xF000, 0x7000}); // 7XNN
	_opCodeMasks.push_back({0xF00F, 0x8000}); // 8XY0 -- 10
	_opCodeMasks.push_back({0xF00F, 0x8001}); // 8XY1
	_opCodeMasks.push_back({0xF00F, 0x8002}); // 8XY2
	_opCodeMasks.push_back({0xF00F, 0x8003}); // 8XY3
	_opCodeMasks.push_back({0xF00F, 0x8004}); // 8XY4
	_opCodeMasks.push_back({0xF00F, 0x8005}); // 8XY5 -- 15
	_opCodeMasks.push_back({0xF00F, 0x8006}); // 8XY6
	_opCodeMasks.push_back({0xF00F, 0x8007}); // 8XY7
	_opCodeMasks.push_back({0xF00F, 0x800E}); // 8XYE
	_opCodeMasks.push_back({0xF00F, 0x9000}); // 9XY0
	_opCodeMasks.push_back({0xF000, 0xA000}); // ANNN -- 20
	_opCodeMasks.push_back({0xF000, 0xB000}); // BNNN
	_opCodeMasks.push_back({0xF000, 0xC000}); // CXNN
	_opCodeMasks.push_back({0xF000, 0xD000}); // DXYN
	_opCodeMasks.push_back({0xF0FF, 0xE09E}); // EX9E
	_opCodeMasks.push_back({0xF0FF, 0xE0A1}); // EXA1 -- 25
	_opCodeMasks.push_back({0xF0FF, 0xF007}); // FX07
	_opCodeMasks.push_back({0xF0FF, 0xF00A}); // FX0A
	_opCodeMasks.push_back({0xF0FF, 0xF015}); // FX15
	_opCodeMasks.push_back({0xF0FF, 0xF018}); // FX18
	_opCodeMasks.push_back({0xF0FF, 0xF01E}); // FX1E -- 30
	_opCodeMasks.push_back({0xF0FF, 0xF029}); // FX29
	_opCodeMasks.push_back({0xF0FF, 0xF033}); // FX33
	_opCodeMasks.push_back({0xF0FF, 0xF055}); // FX55
	_opCodeMasks.push_back({0xF0FF, 0xF065}); // FX65 -- 34
}
















