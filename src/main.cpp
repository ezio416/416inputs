// c 2023-12-10
// m 2023-12-11

#include <iostream>
#include <toml.hpp>
#include <SDL.h>

bool always_on_top = false;
bool borderless = false;
int window_width = 640;
int window_height = 480;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

bool ReadConfig() {
    toml::table config;
    try {
        config = toml::parse_file("main.toml");
    } catch (const toml::parse_error& err) {
        std::cerr << err << "\n";
        return false;
    }

    always_on_top = (bool)config["window"]["always_on_top"].value<bool>().value();
    borderless = (bool)config["window"]["borderless"].value<bool>().value();
    window_width = (int)config["window"]["resolution"]["width"].value<int>().value();
    window_height = (int)config["window"]["resolution"]["height"].value<int>().value();

    printf("configured resolution: %ix%i\n", window_width, window_height);

    return true;
}

void SetRendererWhite() {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

bool Init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL failed to initialize: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
        printf("warning: linear texture filtering disabled");

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (always_on_top)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;

    window = SDL_CreateWindow(
        "416inputs",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width,
        window_height,
        flags
    );

    if (window == NULL) {
        printf("window failed to create: %s\n", SDL_GetError());
        return false;
    }

    if (borderless)
        SDL_SetWindowBordered(window, SDL_bool::SDL_FALSE);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == NULL) {
        printf("renderer failed to create: %s\n", SDL_GetError());
        return false;
    }

    SetRendererWhite();

    return true;
}

void Close() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    renderer = NULL;
    window = NULL;

    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (!ReadConfig()) {
        printf("default resolution: %ix%i\n", window_width, window_height);
        // return 1;
    }

    if (!Init()) {
        Close();
        return 1;
    }

    SDL_Event e;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT)
                quit = true;
        }

        SetRendererWhite();
        SDL_RenderClear(renderer);

        SDL_Rect fillRect = {
            window_width / 4,
            window_height / 4,
            window_width / 2,
            window_height / 2
        };
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        SDL_RenderFillRect(renderer, &fillRect);

        SDL_RenderPresent(renderer);
    }

    Close();
    return 0;
}