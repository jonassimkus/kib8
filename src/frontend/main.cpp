#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>  

#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "core/cpu.cpp"

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

    CPU cpu = CPU();

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
        }

        cpu.write(0, cpu.read(0)+1);

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Memory Table");
        ImGui::Text("Memory 0x000: %d", cpu.read(0));
        ImGui::End();

        ImGui::Render();

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
            pixel.y = 8 * (pixel_y);
            SDL_RenderFillRect(renderer, &pixel);
        }

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}