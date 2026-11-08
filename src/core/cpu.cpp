#include <X11/Xmd.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stack> 
#include <unordered_map>

class CPU {
    public:

    std::unordered_map<uint16_t, int> key_offsets = {
        {30, 1},
        {31, 2},
        {32, 3},
        {34,12},
        {20,4},
        {26,5},
        {8,6},
        {21,13},
        {4,7},
        {22,8},
        {7,9},
        {9,14},
        {29,10},
        {27,0},
        {6,11},
        {25,15}
    };

    uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    
    uint8_t memory[4096] = {};
    uint16_t index_register = 0;
    uint16_t program_counter = 0;
    uint8_t v_register[16] = {};
    std::stack<uint16_t> stack;
    BYTE delay_timer = 0;
    BYTE sound_timer = 0; 

    bool screen[64*32];

    void set_pixel(int x, int y, bool val){
        screen[y * 64 + x] = val;
    }

    bool get_pixel(int x, int y){
        return screen[y * 64 + x];
    }

    void update_sound_timer(){
        if (sound_timer > 0) {
            sound_timer -= 1;
        }
    }

    void update_delay_timer(){
        if (delay_timer > 0){
            delay_timer -= 1;
        }
    }

    void draw_letter(int scancode){
        
    }

    void send_key_press(int scancode){
    }

    int load_rom(const uint8_t* rom, size_t size){
        const int start_address = 512;
        for (int i = 0; i < 4096-start_address; ++i){
            memory[start_address+i] = 0;
        }

        program_counter = start_address;
        stack = std::stack<uint16_t>();
        v_register[16] = {};

        for (int i = 0; i < size; ++i){
            memory[start_address+i] = rom[i];
            //std::cout << (int)rom[i] << std::endl;
        }

        clear_screen();

        return 0;
    }

    uint8_t program_counter_read(){
        uint8_t byte;
        byte = memory[program_counter];
        return byte;
    }

    int read(int addr){
        if (addr >= 4096 || addr < 0){
            std::cout << "Memory Address " << addr << " out of range!" << std::endl;
            return 0;
        }
        return memory[addr];
    }

    void write(int addr, int val){
        if (addr >= 4096 || addr < 0){
            std::cout << "Memory Address " << addr << " out of range!" << std::endl;
            return;
        }
        memory[addr] = val;
    }

    void clear_screen(){
         for (int i = 0; i < 64*32; ++ i){
            screen[i] = 0;
        }
    }

    uint16_t fetch(){
        uint8_t first_mem = memory[program_counter];
        uint8_t second_mem = memory[program_counter+1];

        uint16_t opcode = (first_mem << 8) | second_mem;
        program_counter += 2;
        return opcode;
    }

    void execute(uint16_t opcode){
        int nibble = (opcode >> 12);
        int X = (opcode >> 8) & 0xf;
        int Y = (opcode >> 4) & 0xf;
        int N = (opcode & 0xf);
        int NN = (opcode & 0xff);
        int NNN = (opcode & 0xfff);

        int x = v_register[X&63];
        int y = v_register[Y&31];

        switch (nibble) {
            case 0x1:
            program_counter = NNN;
            break;

            case 0x0:
                if (NN == 0xE0){
                    clear_screen();
                }
            break;
    
            case 0xA:
            index_register = NNN;
            break;

            case 0x6:
            v_register[X] = NN;
            break;

            case 0x7:
            v_register[X] += NN;
            break;

            case 0xD:
            v_register[0xF] = 0;
            if (v_register[0xF] == 0){
                int x = v_register[X];
                int y = v_register[Y];
                for (int i = 0; i < N; ++i){
                    int sprite_data = memory[index_register+i];
                    for (int j = 0; j < 8; ++j){
                        if ((x + j) > 63){
                            continue;
                        }

                        int sprite_bit = (sprite_data >> (7 - j)) & 1;

                        bool is_on = get_pixel(x + j, y+i);
                        if (is_on && sprite_bit == 1){
                            set_pixel(x + j,  y+i, false);
                            v_register[0xF] = 1;
                        }
                        else if (!is_on && sprite_bit == 1){
                            set_pixel(x + j,  y+i, true);
                        }

                    }
                }
            }
            break;

            default:
            std::cout << "Opcode not implemented!" << std::endl;
            break;
        }
    }

    void tick(){
        // waiting for rom menu
        if (program_counter < 512){
            set_pixel(10, 10, true);
            set_pixel(10, 11, true);
            set_pixel(10, 12, true);
            set_pixel(10, 13, true);
            set_pixel(10, 14, true);
            set_pixel(10, 15, true);

            set_pixel(15, 10, true);
            set_pixel(15, 11, true);
            set_pixel(15, 12, true);
            set_pixel(15, 13, true);
            set_pixel(15, 14, true);
            set_pixel(15, 15, true);
            return;
        }

        // same pause menu for now, error memory overflow
        if (program_counter >= 4096){
            set_pixel(10, 10, true);
            set_pixel(10, 11, true);
            set_pixel(10, 12, true);
            set_pixel(10, 13, true);
            set_pixel(10, 14, true);
            set_pixel(10, 15, true);

            set_pixel(15, 10, true);
            set_pixel(15, 11, true);
            set_pixel(15, 12, true);
            set_pixel(15, 13, true);
            set_pixel(15, 14, true);
            set_pixel(15, 15, true);
            return;
        }

        uint16_t opcode = fetch();
        printf("%04X\n", opcode);
        execute(opcode);
    }
};