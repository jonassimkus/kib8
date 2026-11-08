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

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("kibnes", 1280, 720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
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

    SDL_FRect pixel = {
        0,0,8,8
    };

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
                cpu.send_key_press(event.key.scancode);
            }

            if (event.type == SDL_EVENT_DROP_FILE){
                const char* dropped_fileDir = event.drop.data;
                std::cout << dropped_fileDir << std::endl;
                // Do something with dropped_fileDir here...

                // Free the string allocated by SDL
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

            pixel.x = 8 * (pixel_x) + 16;
            pixel.y = 8 * (pixel_y) + 32;
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

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::Render();

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        SDL_DelayNS(SDL_MS_TO_NS(1.43));
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}