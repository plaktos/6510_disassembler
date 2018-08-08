#include <iostream>
#include <iomanip>
#include <fstream>

// RAII for FILE* pointers
struct FileHandle{
    FileHandle(char* fn, char* mode){
        file = fopen(fn, mode);
    }
    ~FileHandle(){
        fclose(file);
    }
    FILE* operator()(){ return file; }

    FILE* file;
};

struct UCharArr{
    UCharArr(int s){
        arr = new unsigned char[s];
    }
    ~UCharArr(){
        delete[] arr;
    }

    unsigned char& operator[](size_t i){
        return arr[i];
    }

    unsigned char* arr;
};

class HexFormatter{
    public:
        HexFormatter(int p) : padby(p) {;}
        friend std::ostream& operator<<(std::ostream& os, const HexFormatter& h);
    private:
        int padby = 0;
};

class OPCodeFormatter{
    public:
        OPCodeFormatter(std:: string a, std::string c, int b = 1, int cx = -1, int cy = -1)
            : command(c), addressing(a), bytes(b), x(cx), y(cy) {;}

        friend std::ostream& operator<<(std::ostream& os, const OPCodeFormatter& c);

    private:
        std::string command, addressing;
        int bytes, x, y;
};

std::ostream& operator<<(std::ostream& os, const HexFormatter& h){
    os << std::hex << std::setw(h.padby) << std::setfill('0');
    return os;
}

std::ostream& operator<<(std::ostream& os, const OPCodeFormatter& c){
    using namespace std;

    if(c.bytes > 3 || c.bytes < 1
            || (c.bytes == 2 && c.x == -1)
            || (c.bytes == 3 && (c.x == -1 || c.y == -1))){
        cerr << "Invalid call to OPToStream\n";
        return os; 
    }
                        
    os << setfill(' ');
    if(c.bytes == 1)      
        os << setw(4) << c.addressing << setw(16) << c.command;
    else if(c.bytes == 2) 
        os << setw(4) << c.addressing << "  " << setw(2) << HexFormatter(2) << c.x << setfill(' ') <<  setw(12) << c.command;
    else if(c.bytes == 3) 
        os << setw(4) << c.addressing << "  " <<  setw(2) << HexFormatter(2) << c.x << setfill(' ') << " " << setw(2) << HexFormatter(2) << c.y
            << setfill(' ') << setw(9) << c.command;
    return os;     
}                       

// 6510 Disassembler    
// OPCodes: https://www.c64-wiki.com/wiki/Opcode
void DisAss_6510(std::fstream& is, std::fstream& os){
    using namespace std ;
    size_t pc = 0; // program counter

    is.seekg(0, ios::end);
    size_t buffsize = is.tellg();
    is.seekg(0, ios::beg);
    UCharArr buffer(buffsize + 1);
    is.read((char*)buffer.arr, buffsize); buffer[buffsize] = '\0';
                        
    // Peek the next op code and read bytes depending on which opcode it is
    // The MOS 6510 only has instructions of size 1,2,3 bytes
    // If peek returns 0 there is nothing to read anymore
    while(pc < buffsize){
        unsigned char* currOPC = &buffer[pc];
        os << HexFormatter(4) << pc;
        // First check  1 byte opcodes
        switch(*currOPC){
            case 0x00: os << OPCodeFormatter("imp","BRK"); pc++; break; // BReaK
            case 0x08: os << OPCodeFormatter("imp","PHP"); pc++; break; // PusH Processor flags
            case 0x0A: os << OPCodeFormatter("akk","ASL"); pc++; break; // Arithmetic Shift Left
            case 0x18: os << OPCodeFormatter("imp","CLC"); pc++; break; // CLear Carry
            case 0x28: os << OPCodeFormatter("imp","PLP"); pc++; break; // PuLl Processor flags
            case 0x2A: os << OPCodeFormatter("akk","ROL"); pc++; break; // ROll Left
            case 0x38: os << OPCodeFormatter("imp","SEC"); pc++; break; // SEt Carry
            case 0x40: os << OPCodeFormatter("imp","RTI"); pc++; break; // ReTurn from Interrupt
            case 0x48: os << OPCodeFormatter("imp","PHA"); pc++; break; // PusH Accumulator 
            case 0x4A: os << OPCodeFormatter("akk","LSR"); pc++; break; // Logic Shift Right 
            case 0x58: os << OPCodeFormatter("imp","CLI"); pc++; break; // CLear Interrupt 
            case 0x60: os << OPCodeFormatter("imp","RTS"); pc++; break; // ReTurn from Subrutine 
            case 0x68: os << OPCodeFormatter("imp","PLA"); pc++; break; // PuLl Accumulator
            case 0x6A: os << OPCodeFormatter("akk","ROR"); pc++; break; // ROtate Right
            case 0x78: os << OPCodeFormatter("imp","SEI"); pc++; break; // SEt Interrupt flag
            case 0x88: os << OPCodeFormatter("imp","DEY"); pc++; break; // DEcrease Y
            case 0x8A: os << OPCodeFormatter("imp","TXA"); pc++; break; // Transfer X to A
            case 0x98: os << OPCodeFormatter("imp","TYA"); pc++; break; // Transfer Y to A
            case 0x9A: os << OPCodeFormatter("imp","TXS"); pc++; break; // Transfer X to Stack pointer
            case 0xA8: os << OPCodeFormatter("imp","TAY"); pc++; break; // Transfer A to Y
            case 0xAA: os << OPCodeFormatter("imp","TAX"); pc++; break; // Transfer A to X
            case 0xB8: os << OPCodeFormatter("imp","CLV"); pc++; break; // CLear oVerflow
            case 0xBA: os << OPCodeFormatter("imp","TSX"); pc++; break; // Transfer Stack pointer to X 
            case 0xC8: os << OPCodeFormatter("imp","INY"); pc++; break; // INcrease Y
            case 0xCA: os << OPCodeFormatter("imp","DEX"); pc++; break; // DEcrease X
            case 0xD8: os << OPCodeFormatter("imp","CLD"); pc++; break; // CLear Decimal flag
            case 0xE8: os << OPCodeFormatter("imp","INX"); pc++; break; // INcrease X
            case 0xEA: os << OPCodeFormatter("imp","NOP"); pc++; break; // No OPeration
            case 0xF8: os << OPCodeFormatter("imp","SED"); pc++; break; // SEt Decimal flag

        // Second check 2 byte opcodes
            case 0x01: os << OPCodeFormatter("inx","ORA", 2, buffer[pc+1]); pc=pc+2; break; // logical OR on A
            case 0x05: os << OPCodeFormatter("zp" ,"ORA", 2, buffer[pc+1]); pc=pc+2; break;  // logical OR on A
            case 0x09: os << OPCodeFormatter("imm","ORA", 2, buffer[pc+1]); pc=pc+2; break; // logical OR on A
            case 0x15: os << OPCodeFormatter("zpx","ORA", 2, buffer[pc+1]); pc=pc+2; break; // logical OR on A
            case 0x11: os << OPCodeFormatter("iny","ORA", 2, buffer[pc+1]); pc=pc+2; break; // logical OR on A
            case 0x06: os << OPCodeFormatter("zp" ,"ASL", 2, buffer[pc+1]); pc=pc+2; break;  // Arithmetic Shift Left
            case 0x16: os << OPCodeFormatter("zpx","ASL", 2, buffer[pc+1]); pc=pc+2; break; // Arithmetic Shift Left
            case 0x10: os << OPCodeFormatter("rel","BPL", 2, buffer[pc+1]); pc=pc+2; break; // Branch if PLus
            case 0x21: os << OPCodeFormatter("inx","AND", 2, buffer[pc+1]); pc=pc+2; break; // logical AND
            case 0x25: os << OPCodeFormatter("zp" ,"AND", 2, buffer[pc+1]); pc=pc+2; break;  // logical AND
            case 0x29: os << OPCodeFormatter("imm","AND", 2, buffer[pc+1]); pc=pc+2; break; // logical AND
            case 0x31: os << OPCodeFormatter("iny","AND", 2, buffer[pc+1]); pc=pc+2; break; // logical AND
            case 0x35: os << OPCodeFormatter("zpx","AND", 2, buffer[pc+1]); pc=pc+2; break; // logical AND
            case 0x24: os << OPCodeFormatter("zp" ,"BIT", 2, buffer[pc+1]); pc=pc+2; break;  // BIT test
            case 0x26: os << OPCodeFormatter("zp" ,"ROL", 2, buffer[pc+1]); pc=pc+2; break;  // ROll Left
            case 0x36: os << OPCodeFormatter("zpx","ROL", 2, buffer[pc+1]); pc=pc+2; break; // ROll Left
            case 0x30: os << OPCodeFormatter("rel","BMI", 2, buffer[pc+1]); pc=pc+2; break; // Branch if MInus
            case 0x49: os << OPCodeFormatter("imm","EOR", 2, buffer[pc+1]); pc=pc+2; break; // Exclusive OR
            case 0x45: os << OPCodeFormatter("zp" ,"EOR", 2, buffer[pc+1]); pc=pc+2; break;  // Exclusive OR 
            case 0x55: os << OPCodeFormatter("zpx","EOR", 2, buffer[pc+1]); pc=pc+2; break; // Exclusive OR 
            case 0x41: os << OPCodeFormatter("inx","EOR", 2, buffer[pc+1]); pc=pc+2; break; // Exclusive OR 
            case 0x51: os << OPCodeFormatter("iny","EOR", 2, buffer[pc+1]); pc=pc+2; break; // Exclusive OR 
            case 0x46: os << OPCodeFormatter("zp" ,"LSR", 2, buffer[pc+1]); pc=pc+2; break;  // Logic Shift Right
            case 0x56: os << OPCodeFormatter("zpx","LSR", 2, buffer[pc+1]); pc=pc+2; break; // Logic Shift Right
            case 0x50: os << OPCodeFormatter("rel","BVC", 2, buffer[pc+1]); pc=pc+2; break; // Branch if oVerflow Clear
            case 0x61: os << OPCodeFormatter("inx","ADC", 2, buffer[pc+1]); pc=pc+2; break; // ADd with Carry
            case 0x65: os << OPCodeFormatter("zp" ,"ADC", 2, buffer[pc+1]); pc=pc+2; break;  // ADd with Carry
            case 0x69: os << OPCodeFormatter("imm","ADC", 2, buffer[pc+1]); pc=pc+2; break; // ADd with Carry
            case 0x71: os << OPCodeFormatter("iny","ADC", 2, buffer[pc+1]); pc=pc+2; break; // ADd with Carry
            case 0x75: os << OPCodeFormatter("zpx","ADC", 2, buffer[pc+1]); pc=pc+2; break; // ADd with Carry
            case 0x66: os << OPCodeFormatter("zp" ,"ROR", 2, buffer[pc+1]); pc=pc+2; break;  // ROtate Right
            case 0x76: os << OPCodeFormatter("zpx","ROR", 2, buffer[pc+1]); pc=pc+2; break; // ROtate Right
            case 0x70: os << OPCodeFormatter("rel","BVS", 2, buffer[pc+1]); pc=pc+2; break; // Branch if oVerflow Set
            case 0x85: os << OPCodeFormatter("zp" ,"STA", 2, buffer[pc+1]); pc=pc+2; break;  // STore Accumulator
            case 0x95: os << OPCodeFormatter("zpx","STA", 2, buffer[pc+1]); pc=pc+2; break; // STore Accumulator
            case 0x81: os << OPCodeFormatter("inx","STA", 2, buffer[pc+1]); pc=pc+2; break; // STore Accumulator
            case 0x91: os << OPCodeFormatter("iny","STA", 2, buffer[pc+1]); pc=pc+2; break; // STore Accumulator
            case 0x84: os << OPCodeFormatter("zp" ,"STY", 2, buffer[pc+1]); pc=pc+2; break;  // STore Y
            case 0x94: os << OPCodeFormatter("zpx","STY", 2, buffer[pc+1]); pc=pc+2; break; // STore Y
            case 0x86: os << OPCodeFormatter("zp" ,"STX", 2, buffer[pc+1]); pc=pc+2; break;  // STore X
            case 0x96: os << OPCodeFormatter("zpx","STX", 2, buffer[pc+1]); pc=pc+2; break; // STore X
            case 0x90: os << OPCodeFormatter("rel","BCC", 2, buffer[pc+1]); pc=pc+2; break; // Break if Carry Clear
            case 0xA0: os << OPCodeFormatter("imm","LDY", 2, buffer[pc+1]); pc=pc+2; break; // LoaD Y
            case 0xA4: os << OPCodeFormatter("zp" ,"LDY", 2, buffer[pc+1]); pc=pc+2; break;  // LoaD Y
            case 0xB4: os << OPCodeFormatter("zpx","LDY", 2, buffer[pc+1]); pc=pc+2; break; // LoaD Y
            case 0xA9: os << OPCodeFormatter("imm","LDA", 2, buffer[pc+1]); pc=pc+2; break; // LoaD A
            case 0xA5: os << OPCodeFormatter("zp" ,"LDA", 2, buffer[pc+1]); pc=pc+2; break;  // LoaD A
            case 0xB5: os << OPCodeFormatter("zpx","LDA", 2, buffer[pc+1]); pc=pc+2; break; // LoaD A
            case 0xA1: os << OPCodeFormatter("inx","LDA", 2, buffer[pc+1]); pc=pc+2; break; // LoaD A
            case 0xB1: os << OPCodeFormatter("iny","LDA", 2, buffer[pc+1]); pc=pc+2; break; // LoaD A
            case 0xA2: os << OPCodeFormatter("imm","LDX", 2, buffer[pc+1]); pc=pc+2; break; // LoaD X
            case 0xA6: os << OPCodeFormatter("zp" ,"LDX", 2, buffer[pc+1]); pc=pc+2; break;  // LoaD X
            case 0xB6: os << OPCodeFormatter("zpx","LDX", 2, buffer[pc+1]); pc=pc+2; break; // LoaD X
            case 0xB0: os << OPCodeFormatter("rel","BCS", 2, buffer[pc+1]); pc=pc+2; break; // Break if Carry Set
            case 0xC0: os << OPCodeFormatter("imm","CPY", 2, buffer[pc+1]); pc=pc+2; break; // ComPare Y
            case 0xC4: os << OPCodeFormatter("zp" ,"CPY", 2, buffer[pc+1]); pc=pc+2; break;  // ComPare Y
            case 0xD0: os << OPCodeFormatter("rel","BNE", 2, buffer[pc+1]); pc=pc+2; break; // Branch if Not Equal
            case 0xC9: os << OPCodeFormatter("imm","CMP", 2, buffer[pc+1]); pc=pc+2; break; // CoMPare
            case 0xC5: os << OPCodeFormatter("zp" ,"CMP", 2, buffer[pc+1]); pc=pc+2; break;  // CoMPare
            case 0xD5: os << OPCodeFormatter("zpx","CMP", 2, buffer[pc+1]); pc=pc+2; break; // CoMPare
            case 0xC1: os << OPCodeFormatter("inx","CMP", 2, buffer[pc+1]); pc=pc+2; break; // CoMPare
            case 0xD1: os << OPCodeFormatter("iny","CMP", 2, buffer[pc+1]); pc=pc+2; break; // CoMPare
            case 0xC6: os << OPCodeFormatter("zp" ,"DEC", 2, buffer[pc+1]); pc=pc+2; break;  // DECrease (address)
            case 0xD6: os << OPCodeFormatter("zpx","DEC", 2, buffer[pc+1]); pc=pc+2; break; // DECrease (address)
            case 0xE0: os << OPCodeFormatter("imm","CPX", 2, buffer[pc+1]); pc=pc+2; break; // ComPare X
            case 0xE4: os << OPCodeFormatter("zp" ,"CPX", 2, buffer[pc+1]); pc=pc+2; break;  // ComPare X
            case 0xE9: os << OPCodeFormatter("imm","SBC", 2, buffer[pc+1]); pc=pc+2; break; // SuBbtract with Carry
            case 0xE5: os << OPCodeFormatter("zp" ,"SBC", 2, buffer[pc+1]); pc=pc+2; break;  // SuBbtract with Carry
            case 0xF5: os << OPCodeFormatter("zpx","SBC", 2, buffer[pc+1]); pc=pc+2; break; // SuBbtract with Carry
            case 0xE1: os << OPCodeFormatter("inx","SBC", 2, buffer[pc+1]); pc=pc+2; break; // SuBbtract with Carry
            case 0xF1: os << OPCodeFormatter("iny","SBC", 2, buffer[pc+1]); pc=pc+2; break; // SuBbtract with Carry
            case 0xE6: os << OPCodeFormatter("zp" ,"INC", 2, buffer[pc+1]); pc=pc+2; break;  // INCrement
            case 0xF6: os << OPCodeFormatter("zpx","INC", 2, buffer[pc+1]); pc=pc+2; break; // INCrement
            case 0xF0: os << OPCodeFormatter("rel","BEQ", 2, buffer[pc+1]); pc=pc+2; break; // Branch if EQual

        // Third check 3 byte opcodes
            case 0x0D: os << OPCodeFormatter("abs","ORA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Logical OR on Accumulator
            case 0x1D: os << OPCodeFormatter("abx","ORA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Logical OR on Accumulator
            case 0x19: os << OPCodeFormatter("aby","ORA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Logical OR on Accumulator
            case 0x0E: os << OPCodeFormatter("abs","ASL", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Arithmetic Shift Left
            case 0x1E: os << OPCodeFormatter("abx","ASL", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Arithmetic Shift Left
            case 0x20: os << OPCodeFormatter("abs","JSR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Jump to SubRoutine
            case 0x2C: os << OPCodeFormatter("abs","BIT", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // BIT test
            case 0x2D: os << OPCodeFormatter("abs","AND", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // logic AND
            case 0x3D: os << OPCodeFormatter("abx","AND", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // logic AND
            case 0x39: os << OPCodeFormatter("aby","AND", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // logic AND
            case 0x2E: os << OPCodeFormatter("abs","ROL", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ROll Left
            case 0x3E: os << OPCodeFormatter("abx","ROL", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ROll Left
            case 0x4C: os << OPCodeFormatter("abs","JMP", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // JuMP
            case 0x4D: os << OPCodeFormatter("abs","EOR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Exclusive OR
            case 0x5D: os << OPCodeFormatter("abx","EOR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Exclusive OR
            case 0x59: os << OPCodeFormatter("aby","EOR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Exclusive OR
            case 0x4E: os << OPCodeFormatter("abs","LSR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Logic Shift Right
            case 0x5E: os << OPCodeFormatter("abx","LSR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // Logic Shift Right
            case 0x6D: os << OPCodeFormatter("abs","ADC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ADd with Carry
            case 0x7D: os << OPCodeFormatter("abx","ADC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ADd with Carry
            case 0x79: os << OPCodeFormatter("aby","ADC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ADd with Carry
            case 0x6E: os << OPCodeFormatter("abs","ROR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ROtate Right
            case 0x7E: os << OPCodeFormatter("abx","ROR", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ROtate Right
            case 0x8C: os << OPCodeFormatter("abs","STY", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // STore Y
            case 0x8E: os << OPCodeFormatter("abs","STX", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // STore X
            case 0x8D: os << OPCodeFormatter("abs","STA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // STore A
            case 0x9D: os << OPCodeFormatter("abx","STA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // STore A
            case 0x99: os << OPCodeFormatter("aby","STA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // STore A
            case 0xAC: os << OPCodeFormatter("abs","LDY", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD Y
            case 0xBC: os << OPCodeFormatter("abx","LDY", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD Y
            case 0xAE: os << OPCodeFormatter("abs","LDX", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD X
            case 0xBE: os << OPCodeFormatter("aby","LDX", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD X
            case 0xAD: os << OPCodeFormatter("abs","LDA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD A
            case 0xBD: os << OPCodeFormatter("abx","LDA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD A
            case 0xB9: os << OPCodeFormatter("aby","LDA", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // LoaD A
            case 0xCC: os << OPCodeFormatter("abs","CPY", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ComPare Y
            case 0xCD: os << OPCodeFormatter("abs","CMP", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ComPare (accumulator)
            case 0xDD: os << OPCodeFormatter("abx","CMP", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ComPare (accumulator)
            case 0xD9: os << OPCodeFormatter("aby","CMP", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ComPare (accumulator)
            case 0xEC: os << OPCodeFormatter("abs","CPX", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // ComPare X
            case 0xCE: os << OPCodeFormatter("abs","DEC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // DECrease
            case 0xDE: os << OPCodeFormatter("abx","DEC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // DECrease
            case 0xED: os << OPCodeFormatter("abs","SBC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // SuBtract with Carry
            case 0xFD: os << OPCodeFormatter("abx","SBC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // SuBtract with Carry
            case 0xF9: os << OPCodeFormatter("aby","SBC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // SuBtract with Carry
            case 0xEE: os << OPCodeFormatter("abs","INC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // INCrement
            case 0xFE: os << OPCodeFormatter("abx","INC", 3, buffer[pc+1], buffer[pc+2]); pc=pc+3; break; // INCrement
            default:
                       os << OPCodeFormatter("UNK", "UNK"); pc++; break;
        }
        os << std::endl;
    }
}

int main(int argc, char** argv){
    std::fstream fi(argv[1], std::ios::binary | std::ios::in);
    std::fstream fo(argv[2], std::ios::binary | std::ios::out);
    DisAss_6510(fi, fo);
}





































