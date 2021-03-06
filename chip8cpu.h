#ifndef CHIP8CPU_H
#define CHIP8CPU_H

#include <vector>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>

using namespace std;


typedef struct opCodeMask{
	uint16_t mask;
	uint16_t val;
	} opCodeMask;



class Chip8cpu{

public :
	static const uint8_t	 _S_HEIGHT=32; // height
	static const uint8_t	 _S_WIDTH =64; // and width of the screen
	static const uint16_t    _IPS     =250; // cpu clock : instruction per second

	Chip8cpu();
	~Chip8cpu();
	int loadRom(const vector<uint8_t>);
	int loadRom(const string filename);
	int step(); // execute one full cpu step
	uint8_t getPix(int); // access a screen pixel
	void setKey(uint, bool); // set a key from the keyboard
	
private :
	vector<uint8_t>  _mem; // RAM
	vector<uint8_t>  _reg; // registers
	uint16_t         _I;   // specific memory register
	uint8_t          _regDelay; // specific delay register
	uint8_t          _regSound; // specific sound timer register
	uint16_t         _PC; // program counter
	uint8_t          _SP; // stack pointer
	vector<uint16_t> _stack; // hold the cpu stack
	vector<uint8_t>  _screen; // 64*32 2D screen as a single vector
	vector<opCodeMask> _opCodeMasks;
	vector<bool>     _keyboard;
	bool             _waitKey; // tells wether we are waiting for a key press
	uint8_t          _waitKeyReg; // register to put the awaited key pressed ID
	
	
	void updateTimers(); // update delay and sound timer
	void initOpCodeMasks(); // create masks for opcode identification
	uint16_t readInstruction(); // read next instruction (2 uint8_t, merged in one uint16_t)
	int decodeInstruction(uint16_t); // decode instruction
	bool executeInstruction(int, uint16_t); // execute given instruction
	void printStack(); // print full stack status
	void printReg(); // print full register status
	void loadFont(); // load specific font related memory
};




#endif // CHIP8CPU_H
