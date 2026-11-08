#include <X11/Xmd.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stack>
#include <SDL3/SDL.h>

class CPU {
    public:

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
    std::mt19937 mt{std::random_device{}()};

    bool screen[64*32] = {};
    bool crash_flag = false;

    int keys_pressed[16] = {};

    int instructions_per_frame = 10;
    int chip_state = 0;
    const int IDLE = 0;
    const int RUNNING = 1;
    const int CRASHED = 2;

    CPU(){
        for (int i = 0; i < 80; ++i){
            memory[0x050+i] = chip8_fontset[i];
        }
    }

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

    void send_key_press(int scancode){
        if (scancode < 0 || scancode > 15){
            return;
        }
        keys_pressed[scancode] = 1;
    }

    void send_key_release(int scancode){
        if (scancode < 0 || scancode > 15){
            return;
        }
        keys_pressed[scancode] = 0;
    }

    void reset(){
        const int start_address = 512;
        for (int i = 0; i < 4096-start_address; ++i){
            memory[start_address+i] = 0;
        }

        chip_state = IDLE;

        program_counter = 0;
        stack = std::stack<uint16_t>();

        for (int i = 0; i < 16; ++i){
            v_register[i] = 0;
        }

        index_register = 0;

        delay_timer = 0;
        sound_timer = 0;

        clear_screen();
    }

    void close_rom(){
        reset();
    }

    int load_rom(const uint8_t* rom, size_t size){
        if (size > 3584){
            SDL_Log("Rom size bigger than 3584 bytes!");
            return -1;
        }

        reset();

        const int start_address = 512;

        chip_state = RUNNING;

        for (int i = 0; i < size; ++i){
            memory[start_address+i] = rom[i];
        }
        program_counter = 512;

        return 0;
    }

    uint8_t program_counter_read(){
        uint8_t byte;
        byte = memory[program_counter & 0xFFF];
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
        uint8_t first_mem = memory[program_counter & 0xFFF];
        uint8_t second_mem = memory[(program_counter+1) & 0xFFF];

        uint16_t opcode = (first_mem << 8) | second_mem;
        program_counter = (program_counter + 2) & 0xFFF;
        return opcode;
    }

    void execute(uint16_t opcode){
        int nibble = (opcode >> 12);
        int X = (opcode >> 8) & 0xf;
        int Y = (opcode >> 4) & 0xf;
        int N = (opcode & 0xf);
        int NN = (opcode & 0xff);
        int NNN = (opcode & 0xfff);

        switch (nibble) {
            case 0x1:
            program_counter = NNN;
            break;

            case 0x0:
                if (NNN == 0x0E0){
                    clear_screen();
                    break;
                }

                if (NNN == 0x0EE){
                    if (stack.empty()){
                        SDL_LogError(0, "0x00EE was called but stack is empty!");
                        chip_state = CRASHED;
                        return;
                    }

                    program_counter = stack.top();
                    stack.pop();
                    break;
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

            case 0xC:{

            v_register[X] = mt() & NN;
            }
            break;

            case 0x3:
            if (v_register[X] == NN){
                program_counter += 2;
            }
            break;

            case 0x4:
            if (v_register[X] != NN){
                program_counter += 2;
            }
            break;

            case 0x2:
            if (stack.size() >= 16){
                SDL_LogError(0, "0x2NNN was called but stack is full!");
                chip_state = CRASHED;
                return;
            }
            stack.push(program_counter);
            program_counter = NNN;
            break;

            case 0x8:
            if(N == 0x0){
                v_register[X] = v_register[Y];
                break;
            }

            if (N == 0x7){
                int flag = 0;
                if (v_register[Y] >= v_register[X]){
                    flag = 1;
                }

                v_register[X] = v_register[Y] - v_register[X];
                v_register[0xF] = flag;
                break;
            }

            if (N == 0x4){
                int flag = 0;
                if (v_register[X] + v_register[Y] > 255){
                    flag = 1;
                }

                v_register[X] = v_register[X] + v_register[Y];
                v_register[0xF] = flag;
                break;
            }

            if (N == 0x1){
                v_register[X] = v_register[X] | v_register[Y];
                break;
            }

            if (N == 0x2){
                v_register[X] = v_register[X] & v_register[Y];
                break;
            }

            if (N == 0x3){
                v_register[X] = v_register[X] ^ v_register[Y];
                break;
            }

            if (N == 0x5){
                int flag = 0;
                if (v_register[X] >= v_register[Y]){
                    flag = 1;
                }

                v_register[X] = v_register[X] - v_register[Y];
                v_register[0xF] = flag;
                break;
            }

            if (N == 0xE){
                int carry = (v_register[Y] >> 7) & 1;
                v_register[X] = v_register[Y] << 1;

                v_register[0xF] = carry;
                break;
            }

            if (N == 0x6){
                int carry = v_register[Y] & 1;
                v_register[X] = v_register[Y] >> 1;
                v_register[0xF] = carry;
                break;
            }

            std::cout << "Opcode not implemented!" << std::endl;
            printf("%04X\n", opcode);
            chip_state = CRASHED;
            break;

            case 0xE:
            if (NN == 0xA1){
                if (!keys_pressed[v_register[X] & 0xF]){
                    program_counter += 2;
                }
                break;
            }

            if (NN == 0x9E){
                if (keys_pressed[v_register[X] & 0xF]){
                    program_counter += 2;
                }
                break;
            }
            break;

            case 0xD:{
                v_register[0xF] = 0;
                int x = v_register[X] & 63;
                int y = v_register[Y] & 31;
                for (int i = 0; i < N; ++i){
                    if ((y + i) > 31){
                        break;
                    }

                    int sprite_data = memory[(index_register+i) & 0xFFF];
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

            case 0xF:
            if (NN == 0x1E){
                index_register += v_register[X];
                break;
            }

            if (NN == 0x0A){
                program_counter -= 2;

                for (int i = 0; i < 16; ++i){
                    if (keys_pressed[i] == 1){
                        program_counter += 2;
                        v_register[X] = i;
                        break;
                    }
                }
                break;
            }

            if (NN == 0x33){
                memory[(index_register) & 0xFFF] = v_register[X] / 100;
                memory[(index_register+1) & 0xFFF] = v_register[X] / 10 % 10;
                memory[(index_register+2) & 0xFFF] = v_register[X]  % 10;
                break;
            }

            if (NN == 0x29){
                index_register = 0x050 + (v_register[X] & 0xF) * 5;
                break;
            }

            if (NN == 0x07){
                v_register[X] = delay_timer;
                break;
            }

            if (NN == 0x15){
                delay_timer = v_register[X];
                break;
            }

            if (NN == 0x18){
                sound_timer = v_register[X];
                break;
            }

            if (NN == 0x55){
                for (int i = 0; i <= X; ++i){
                    memory[(index_register+i) & 0xFFF] = v_register[i];
                }
                break;
            }

             if (NN == 0x65){
                for (int i = 0; i <= X; ++i){
                    v_register[i] = memory[(index_register+i) & 0xFFF];
                }
                break;
            }

            break;

            case 0x5:
            if (N == 0x0){
                if (v_register[X] == v_register[Y]){
                    program_counter += 2;
                }
                break;
            }
            break;

            case 0x9:
            if (N == 0x0){
                if (v_register[X] != v_register[Y]){
                    program_counter += 2;
                }
                break;
            }
            break;

            case 0xB:
            program_counter = (NNN + v_register[0]) & 0xFFF;
            break;

            default:
            std::cout << "Opcode not implemented!" << std::endl;
            printf("%04X\n", opcode);
            crash_flag = true;
            break;
        }
    }

    void render_pause(){
        for (int i = 0; i < 12; ++i){
            set_pixel(27, 10+i, true);
            set_pixel(28, 10+i, true);

            set_pixel(36, 10+i, true);
            set_pixel(37, 10+i, true);
        }
    }

    void tick(){
        if (chip_state == IDLE){
            render_pause();
            return;
        }

        if (chip_state == CRASHED || program_counter > 4096){
            render_pause();
            return;
        }

        if (chip_state == RUNNING){
            update_delay_timer();
            update_sound_timer();

            for (int i = 0; i <instructions_per_frame; ++i){
                uint16_t opcode = fetch();
                execute(opcode);
            }

            return;
        }
        //printf("%04X\n", opcode);
    }
};