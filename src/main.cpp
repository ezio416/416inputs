// c 2023-12-10
// m 2023-12-11

#include <iostream>
#include <toml.hpp>
#include <SDL.h>

bool always_on_top = false;
bool borderless = false;
int window_width = 640;
int window_height = 480;
bool use_gamepad = false;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Joystick* gamepad = NULL;

int joy_axis_value = 0;
float joy_axis_value_normalized = 0.0f;

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
    use_gamepad = (bool)config["input"]["use_gamepad"].value<bool>().value();

    printf("configured resolution: %ix%i\n", window_width, window_height);

    return true;
}

bool Init() {
    Uint32 init_flags = SDL_INIT_VIDEO;
    if (use_gamepad)
        init_flags |= SDL_INIT_JOYSTICK;

    if (SDL_Init(init_flags) < 0) {
        printf("SDL failed to initialize: %s\n", SDL_GetError());
        return false;
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
        printf("warning: linear texture filtering disabled");

    if (use_gamepad && !SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1"))
        printf("warning: background joystick failed");

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

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    return true;
}

void JoystickClose() {
    SDL_JoystickClose(gamepad);
    gamepad = NULL;
}

void Close() {
    JoystickClose();

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

    int last_axis_value = 0;
    // int last_width = 0;

    while (!quit) {
        if (use_gamepad) {
            if (SDL_NumJoysticks() == 0)
                JoystickClose();
            else {
                gamepad = SDL_JoystickOpen(0);
                if (gamepad == NULL)
                    printf("warning: failed to load gamepad: %s\n", SDL_GetError());
            }
        } else if (gamepad != NULL)
            JoystickClose();

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT)
                quit = true;
            else if (e.type = SDL_JOYAXISMOTION && e.jaxis.which == 0) {
                if (e.jaxis.axis == 0)
                    joy_axis_value = e.jaxis.value;
                else if (e.jaxis.axis == 1) {
                    ;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x66, 0x66, 0x66, 0xFF);
        SDL_RenderClear(renderer);

        int ww3 = window_width / 3 + 1;
        int wh2 = window_height / 2;

        Uint8 r = 0xFF;  // 1.0
        Uint8 g = 0x33;  // 0.2
        Uint8 b = 0x99;  // 0.6
        Uint8 a = 0xFF;  // 1.0

        SDL_SetRenderDrawColor(renderer, r, g, b, a);

        if (use_gamepad && gamepad != NULL) {
            if (joy_axis_value != last_axis_value) {
                joy_axis_value_normalized = (float)joy_axis_value / 32768;
                // std::cout << joy_axis_value << " " << joy_axis_value_normalized << "\n";
                last_axis_value = joy_axis_value;
            }

            int width = (int)((float)ww3 * fabs(joy_axis_value_normalized));
            // if (width != last_width) {
            //     std::cout << "width: " << width << "\n";
            //     last_width = width;
            // }

            if (joy_axis_value_normalized < 0) {
                SDL_Rect rect = {
                    ww3 - width,
                    0,
                    width,
                    window_height
                };
                SDL_RenderFillRect(renderer, &rect);
            } else if (joy_axis_value_normalized > 0) {
                SDL_Rect rect = {
                    ww3 * 2,
                    0,
                    width,
                    window_height
                };
                SDL_RenderFillRect(renderer, &rect);
            }
        } else {
            const Uint8* keyStates = SDL_GetKeyboardState(NULL);

            if (keyStates[SDL_SCANCODE_W]) {
                SDL_Rect rect = { ww3, 0, ww3, wh2 };
                SDL_RenderFillRect(renderer, &rect);
            }
            if (keyStates[SDL_SCANCODE_A]) {
                SDL_Rect rect = { 0, 0, ww3, window_height };
                SDL_RenderFillRect(renderer, &rect);
            }
            if (keyStates[SDL_SCANCODE_S] || keyStates[SDL_SCANCODE_SPACE]) {
                SDL_Rect rect = { ww3, wh2, ww3, wh2 };
                SDL_RenderFillRect(renderer, &rect);
            }
            if (keyStates[SDL_SCANCODE_D]) {
                SDL_Rect rect = { ww3 * 2 - 1, 0, ww3, window_height };
                SDL_RenderFillRect(renderer, &rect);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

        SDL_Rect outline = { 0, 0, window_width - 1, window_height };  // thicker border
        SDL_RenderDrawRect(renderer, &outline);

        outline = { 1, 1, window_width - 3, window_height - 2 };  // forwards
        SDL_RenderDrawRect(renderer, &outline);

        outline = { ww3, 0, ww3, window_height };  // backwards
        SDL_RenderDrawRect(renderer, &outline);

        SDL_RenderDrawLine(renderer, ww3, wh2, ww3 * 2 - 1, wh2);  // middle

        SDL_RenderPresent(renderer);
    }

    Close();
    return 0;
}