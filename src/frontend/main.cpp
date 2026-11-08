#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>  
#include <cstddef>
#include <cstdint>

#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_timer.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "core/cpu.cpp"
#include "imgui_internal.h"

#include <unordered_map>

CPU cpu = CPU();

static const SDL_DialogFileFilter filters[] = {
    { "ch8",  "ch8" },
    { "All files",   "*" }
};

static void SDLCALL callback(void* userdata, const char* const* filelist, int filter){
    if (!filelist) {
        SDL_Log("An error occured: %s", SDL_GetError());
        return;
    } else if (!*filelist) {
        SDL_Log("The user did not select any file.");
        SDL_Log("Most likely, the dialog was canceled.");
        return;
    }

    while (*filelist) {
        SDL_Log("Full path to selected file: '%s'", *filelist);

        size_t size ;

        void* content{
            SDL_LoadFile(*filelist, &size)
        };

        if (content == NULL){
            SDL_Log("An error occured: %s", SDL_GetError());
            filelist++;
            break;
        }

        const uint8_t* rom = static_cast<const uint8_t*>(content);

        cpu.load_rom(rom, size);
        SDL_free(content);
        filelist++;
    }

    if (filter < 0) {
        SDL_Log("The current platform does not support fetching "
                "the selected filter, or the user did not select"
                " any filter.");
    } else if (filter < SDL_arraysize(filters)) {
        SDL_Log("The filter selected by the user is '%s' (%s).",
                filters[filter].pattern, filters[filter].name);
    }
}

std::unordered_map<uint16_t, uint8_t> key_offsets = {
    {SDL_SCANCODE_1, 0x1}, {SDL_SCANCODE_2, 0x2}, {SDL_SCANCODE_3, 0x3}, {SDL_SCANCODE_4, 0xC},
    {SDL_SCANCODE_Q, 0x4}, {SDL_SCANCODE_W, 0x5}, {SDL_SCANCODE_E, 0x6}, {SDL_SCANCODE_R, 0xD},
    {SDL_SCANCODE_A, 0x7}, {SDL_SCANCODE_S, 0x8}, {SDL_SCANCODE_D, 0x9}, {SDL_SCANCODE_F, 0xE},
    {SDL_SCANCODE_Z, 0xA}, {SDL_SCANCODE_X, 0x0}, {SDL_SCANCODE_C, 0xB}, {SDL_SCANCODE_V, 0xF},
};

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("kibnes", 8*64, 8*33+19,
        SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);


    bool running = true;

    int instructions_per_frame = 10;

    SDL_FRect pixel = {
        0,0,8,8
    };

    SDL_SetWindowSize(window, 8*64, 8*36+ImGui::GetFrameHeight()+6);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);  
            if (event.type == SDL_EVENT_QUIT){
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)){
                running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN){
                if (key_offsets.contains(event.key.scancode)){
                 cpu.send_key_press(key_offsets[event.key.scancode]);
                }
            }

            if (event.type == SDL_EVENT_KEY_UP){
                if (key_offsets.contains(event.key.scancode)){
                 cpu.send_key_release(key_offsets[event.key.scancode]);
                }
            }

            if (event.type == SDL_EVENT_DROP_FILE){
                const char* dropped_fileDir = event.drop.data;

                while (*dropped_fileDir) {
                    size_t size ;

                    void* content{
                        SDL_LoadFile(dropped_fileDir, &size)
                    };

                    if (content == NULL){
                        dropped_fileDir++;
                        break;
                    }

                    const uint8_t* rom = static_cast<const uint8_t*>(content);

                    cpu.load_rom(rom, size);
                    SDL_free(content);
                    dropped_fileDir++;
                }
            }
        }
        
        cpu.tick();
       
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        SDL_SetRenderDrawColor(renderer, 25, 25, 45, 255);
        SDL_RenderClear(renderer);

        for (int i = 0; i < 64*32; ++i){
            int pixel_x = (i%64);
            int pixel_y = (i/64);

            if (cpu.get_pixel(pixel_x, pixel_y)){
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }else{
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }

            pixel.x = 8 * (pixel_x);
            pixel.y = 8 * (pixel_y) + ImGui::GetFrameHeight();

            SDL_RenderFillRect(renderer, &pixel);
        }

    

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                    SDL_ShowOpenFileDialog(*callback, NULL, window, filters, 2, NULL, false);
                }

                if (ImGui::MenuItem("Close", "Ctrl+X")){
                    cpu.close_rom();
                }

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }


        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos({main_viewport->WorkPos.x, main_viewport->WorkPos.y + main_viewport->WorkSize.y - ImGui::GetFrameHeight()-6});
        ImGui::SetNextWindowSize({main_viewport->WorkSize.x, (main_viewport->WorkSize.y - 32*8 - ImGui::GetFrameHeight()+6)});

        ImGui::Begin("status", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
        
        if (cpu.chip_state == 1){
            ImGui::Text("Running");
        }
        if (cpu.chip_state == 0){
            ImGui::Text("Idle");
        }
        if (cpu.chip_state == 2){
            ImGui::Text("Crashed");
        }
        if (cpu.chip_state == 3){
            ImGui::Text("Paused");
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("Instructions per tick", &instructions_per_frame, 0, 100);
        ImGui::SameLine();
        if(ImGui::Button("Pause")){
            if (cpu.chip_state == 1){
                cpu.chip_state = 3;
            }else if(cpu.chip_state == 3){
                cpu.chip_state = 1;
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Step") && cpu.chip_state == 3){
            cpu.update_delay_timer();
            cpu.update_sound_timer();

            for (int i = 0; i < instructions_per_frame; ++i){
                uint16_t opcode = cpu.fetch();
                cpu.execute(opcode);
            }
        }

        ImGui::End();

        ImGui::Render();


        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        SDL_DelayNS(SDL_MS_TO_NS(1.43));

        cpu.instructions_per_frame = instructions_per_frame;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}