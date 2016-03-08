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
                      _stack(vector<uint16_t>(16,0)),
                      _screen(vector<uint8_t>(_S_WIDTH*_S_HEIGHT, 0)),
                      _opCodeMasks(vector<opCodeMask>()), 
                      _keyboard(vector<bool>(16, false)),
                      _waitKey(false),
                      _waitKeyReg(0)                      
{
	initOpCodeMasks();
	loadFont();
}


// destructor
Chip8cpu::~Chip8cpu()
{

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


// access a screen pixel
uint8_t Chip8cpu::getPix(int i)
{
	return _screen[i];
}


// set a key from the keyboard
void Chip8cpu::setKey(uint k, bool val)
{
	_keyboard[k] = val;
}


// execute one full cpu step
int Chip8cpu::step()
{
	if (_waitKey) {
		cout << "waiting for a key..." << endl;
		for (int i=0; i<16; i++) {
			if (_keyboard[i]){
				_reg[_waitKeyReg] = i;
				_waitKey = false;
				break;
			}
		}
	}
	if (!_waitKey) {
		uint16_t ins = readInstruction();
		int insID = decodeInstruction(ins);
		if (insID < 0){
			cerr << _PC << " error while decoding instruction : 0x" 
				 << std::hex << setw(4) << setfill('0')<< ins << std::dec << endl;
			return -1;
		}
		else{
			executeInstruction(insID, ins);
		}
		updateTimers();	
		_PC += 2;
	}
	return 0;
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
		return i;
	}
	else
	{
		return -1;
	}
}

// print full stack, debug fct
void Chip8cpu::printStack()
{
	cout << "[";
	for(int i=0; i<_stack.size()-1; i++) cout << _stack[i] << " ";
	cout << _stack[_stack.size()-1] << "]" << endl;
}

// print registers status, debug fct
void Chip8cpu::printReg()
{
	cout << "[ ";
	for(int i=0; i<_reg.size()-1; i++) cout << (uint)_reg[i] << " ";
	cout << (uint)_reg[_reg.size()-1] << "]" << endl;
}


// execute an instruction based on it's ID
bool Chip8cpu::executeInstruction(int insID, uint16_t ins)
{
	uint8_t X = (ins & 0x0F00) >> 8;
	uint8_t Y = (ins & 0x00F0) >> 4;
	uint8_t N = ins & 0x000F;
	bool success = true;
	int tmp;
	//cout << _PC << " " << "0x" << hex << uppercase << setw(4) << setfill('0') << ins << dec << nouppercase << " " << insID << endl; 
	//printReg();
	switch(insID) 
	{
		case 0: // 0NNN
			// device specific, pass
			//cerr << "instruction 0NNN should not happen !" << endl;
			break;
		
		case 1: // 00E0 clear screen
			_screen.assign(64*32, 0);
			break;
		
		case 2: // 00EE return from subroutine
			if (_SP < 1){
				cerr << "ERROR : stack underflow" << endl;
				success = false;
				break;
			}
			_PC = _stack[_SP-1];
			_SP -= 1;
			//cout << "return from subroutine SP =" << (uint)_SP << endl;
			//printStack();
			break;
		
		case 3: // 1NNN jump to address NNN
			_PC = ((uint16_t)X<<8) + (Y<<4) + N;
			_PC -= 2;
			break;
		
		case 4: // 2NNN call subroutine
			if (_SP >= 15){
				cerr << "ERROR : stack overflow" << endl;
				success = false;
				break;
			}
			_stack[_SP] = _PC;
			_SP += 1;
			_PC = ((uint16_t)X<<8) + (Y<<4) + N;
			_PC -= 2;
			//cout << "0x" << hex << setw(4) << ins << dec << " "; 
			//cout << "call subroutine SP =" << (uint)_SP << endl;
			//printStack();
			break;
			
		case 5: // 3XNN Skip next instruction if Vx = NN
			if (_reg[X] == ((Y<<4) + N) ){
				_PC += 2;
			}
			break;
		
		case 6: // 4XNN Skip next instruction if Vx != NN
			if (_reg[X] != ((Y<<4) + N) ){
				_PC += 2;
			}
			break;
		
		case 7: // 5xy0 Skip next instruction if Vx = Vy.
			if (_reg[X] == _reg[Y]) {
				_PC += 2;
			}
			break;
			
		case 8: // 6xNN Set Vx = NN
			_reg[X] = (Y<<4) + N;
			break;
			
		case 9: // 7xNN Vx = Vx + kk.
			_reg[X] += (Y<<4) + N;
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
			if (tmp > 255) _reg[15] = 1;
			else           _reg[15] = 0;
			break;
		
		case 15: // 8xy5 Set Vx = Vx - Vy, set VF = NOT borrow
			tmp = int(_reg[X]) - _reg[Y];
			if (tmp < 0) _reg[15] = 0;
			else         _reg[15] = 1;
			_reg[X] = tmp % 256;
			break;
			
		case 16: // 8xy6 - Set Vx = Vx SHR 1.
			_reg[15] = _reg[X] & 0x01;
			_reg[X] >>= 1;
			break;
			
		case 17: // 8xy7 Set Vx = Vy - Vx, set VF = NOT borrow.
			tmp = int(_reg[Y]) - _reg[X];
			if (tmp < 0) _reg[15] = 0;
			else         _reg[15] = 1;
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
			_I = ((uint16_t)X<<8) + (Y<<4) + N;
			break;
		
		case 21: // Bnnn Jump to location nnn + V0
			_PC = (((uint16_t)X<<8) + (Y<<4) + N) + _reg[0];
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
		
		case 24: // Ex9E Skip next instruction if key with the value of Vx is pressed.
			if (_keyboard[_reg[X]]){
				_PC += 2;
			}
			break;

		case 25: // Ex9E Skip next instruction if key with the value of Vx is pressed.
			if (! _keyboard[_reg[X]]){
				_PC += 2;
			}
			break;
		
		case 26: // Fx07 Set Vx = delay timer value.
			_reg[X] = _regDelay;
			break;
		
		case 27: // Fx0A Wait for a key press, store the value of the key in Vx.
			_waitKey = true;
			_waitKeyReg = X;
			break;
		
		case 28:
			_regDelay = _reg[X];
			break;
		
		case 29: // Fx18 Set sound timer = Vx.
			_regSound = _reg[X];
			break;
			
		case 30: // Fx1E Set I = I + Vx.
			tmp = (uint16_t)_I + _reg[X];
			_I = tmp % 0xFFF;
			if (tmp > 0xFFF){
				_reg[15] = 1;
			}
			break;
		
		case 31: // Fx29 Set I = location of sprite for digit Vx.
			_I = _reg[X]*5;
			break;

		case 32: // Fx33 Store BCD representation of Vx in memory locations I, I+1, and I+2.
			_mem[_I] = _reg[X] / 100;
			_mem[_I+1] = (_reg[X] / 10) % 10;
			_mem[_I+2] = _reg[X] % 10;
			break;
		
		case 33: // Fx55 Store registers V0 through Vx in memory starting at location I.
			for (int i=0; i<X+1; i++){
				_mem[_I+i] = _reg[i];
			}
			break;
		
		case 34: // Fx65 Read registers V0 through Vx from memory starting at location I.
			for (int i=0; i<X+1; i++){
				_reg[i] = _mem[_I+i];
			}
			break;
			
			
		default :
			cout << "instruction " << insID << " not implemented yet" << endl;
	}
	
	return success;
}




void Chip8cpu::initOpCodeMasks(){
	_opCodeMasks.push_back({0x0000, 0x0FFF}); // 0NNN -- 0
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

void Chip8cpu::loadFont()
{
    _mem[0]=0xF0;_mem[1]=0x90;_mem[2]=0x90;_mem[3]=0x90; _mem[4]=0xF0; // O

    _mem[5]=0x20;_mem[6]=0x60;_mem[7]=0x20;_mem[8]=0x20;_mem[9]=0x70; // 1

    _mem[10]=0xF0;_mem[11]=0x10;_mem[12]=0xF0;_mem[13]=0x80; _mem[14]=0xF0; // 2

    _mem[15]=0xF0;_mem[16]=0x10;_mem[17]=0xF0;_mem[18]=0x10;_mem[19]=0xF0; // 3

    _mem[20]=0x90;_mem[21]=0x90;_mem[22]=0xF0;_mem[23]=0x10;_mem[24]=0x10; // 4

    _mem[25]=0xF0;_mem[26]=0x80;_mem[27]=0xF0;_mem[28]=0x10;_mem[29]=0xF0; // 5

    _mem[30]=0xF0;_mem[31]=0x80;_mem[32]=0xF0;_mem[33]=0x90;_mem[34]=0xF0; // 6

    _mem[35]=0xF0;_mem[36]=0x10;_mem[37]=0x20;_mem[38]=0x40;_mem[39]=0x40; // 7

    _mem[40]=0xF0;_mem[41]=0x90;_mem[42]=0xF0;_mem[43]=0x90;_mem[44]=0xF0; // 8

    _mem[45]=0xF0;_mem[46]=0x90;_mem[47]=0xF0;_mem[48]=0x10;_mem[49]=0xF0; // 9

    _mem[50]=0xF0;_mem[51]=0x90;_mem[52]=0xF0;_mem[53]=0x90;_mem[54]=0x90; // A

    _mem[55]=0xE0;_mem[56]=0x90;_mem[57]=0xE0;_mem[58]=0x90;_mem[59]=0xE0; // B

    _mem[60]=0xF0;_mem[61]=0x80;_mem[62]=0x80;_mem[63]=0x80;_mem[64]=0xF0; // C

    _mem[65]=0xE0;_mem[66]=0x90;_mem[67]=0x90;_mem[68]=0x90;_mem[69]=0xE0; // D

    _mem[70]=0xF0;_mem[71]=0x80;_mem[72]=0xF0;_mem[73]=0x80;_mem[74]=0xF0; // E

    _mem[75]=0xF0;_mem[76]=0x80;_mem[77]=0xF0;_mem[78]=0x80;_mem[79]=0x80; // F

}















