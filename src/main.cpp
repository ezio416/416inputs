// c 2023-12-10
// m 2023-12-11

#include <iostream>
#include <SDL.h>
#include <SDL_ttf.h>
#include <toml.hpp>

using namespace std;

bool always_on_top = false;
bool borderless = false;
int window_width = 640;
int window_height = 480;
int font_size = 20;
bool use_gamepad = false;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Joystick* gamepad = NULL;
SDL_Texture* textTexture = NULL;
TTF_Font* font = NULL;

int textWidth = 0;
int textHeight = 0;
int steer_value = 0;
float steer_value_normalized = 0.0f;
string steer_percent = "";
bool left_trigger = false;
bool right_trigger = false;

bool ReadConfig() {
    toml::table config;
    try {
        config = toml::parse_file("main.toml");
    } catch (const toml::parse_error& err) {
        cerr << err << "\n";
        return false;
    }

    always_on_top = (bool)config["window"]["always_on_top"].value<bool>().value();
    borderless = (bool)config["window"]["borderless"].value<bool>().value();
    window_width = (int)config["window"]["resolution"]["width"].value<int>().value();
    window_height = (int)config["window"]["resolution"]["height"].value<int>().value();
    font_size = (int)config["window"]["text"]["font_size"].value<int>().value();
    use_gamepad = (bool)config["input"]["use_gamepad"].value<bool>().value();

    printf("always_on_top: %s\n", always_on_top ? "true" : "false");
    printf("borderless: %s\n", borderless ? "true" : "false");
    printf("resolution: %ix%i\n", window_width, window_height);
    printf("font_size: %i\n", font_size);
    printf("use_gamepad: %s\n", use_gamepad ? "true" : "false");

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

    if (TTF_Init() < 0) {
        printf("SDL_ttf failed to initialize: %s\n", TTF_GetError());
        return false;
    }

    font = TTF_OpenFont("DroidSans-Bold.ttf", font_size);
    if (font == NULL) {
        printf("font failed to load: %s\n", TTF_GetError());
        return false;
    }

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
        printf("warning: linear texture filtering disabled\n");

    if (use_gamepad && !SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1"))
        printf("warning: background joystick failed\n");

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

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == NULL) {
        printf("renderer failed to create: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    return true;
}

void FreeTexture() {
    if (textTexture == NULL)
        return;

    SDL_DestroyTexture(textTexture);
    textTexture = NULL;
}

bool LoadTextureFromText(string textureText, SDL_Color textColor) {
    FreeTexture();

    SDL_Surface* textSurface = TTF_RenderText_Solid(font, textureText.c_str(), textColor);

    if (textSurface == NULL) {
        printf("text surface failed to create: %s\n", TTF_GetError());
        return false;
    }

    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    if (textTexture == NULL)
        printf("text texture failed to create: %s\n", SDL_GetError());
    else {
        textWidth = textSurface->w;
        textHeight = textSurface->h;
    }

    SDL_FreeSurface(textSurface);

    return textTexture != NULL;
}

void JoystickClose() {
    SDL_JoystickClose(gamepad);
    gamepad = NULL;
}

void Close() {
    JoystickClose();

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    font = NULL;
    renderer = NULL;
    window = NULL;

    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    if (!ReadConfig())
        printf("---reading config file failed---\nalways_on_top: false\nborderless: false\nresolution: %ix%i\nfont_size: %i\nuse_gamepad: false\n", window_width, window_height, font_size);

    if (!Init()) {
        Close();
        return 1;
    }

    SDL_Color textColor = { 0xFF, 0xFF, 0xFF };

    SDL_Event e;
    bool quit = false;

    int last_steer_value = 0;

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
                if (e.jaxis.axis == 0)  // left x
                    steer_value = e.jaxis.value;
                else if (e.jaxis.axis == 4)  // left trigger
                    left_trigger = e.jaxis.value != -32768;
                else if (e.jaxis.axis == 5)  // right trigger
                    right_trigger = e.jaxis.value != -32768;
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
            if (left_trigger) {
                SDL_Rect rect = { ww3, wh2, ww3, wh2 };
                SDL_RenderFillRect(renderer, &rect);
            }
            if (right_trigger) {
                SDL_Rect rect = { ww3, 0, ww3, wh2 };
                SDL_RenderFillRect(renderer, &rect);
            }

            if (steer_value != last_steer_value) {
                steer_value_normalized = (float)steer_value / 32767;
                steer_percent = to_string((int)(abs(steer_value_normalized) * 100));
                last_steer_value = steer_value;
            }

            int width = (int)((float)ww3 * fabs(steer_value_normalized));

            if (steer_value_normalized < 0) {
                SDL_Rect rect = {
                    ww3 - width,
                    0,
                    width,
                    window_height
                };
                SDL_RenderFillRect(renderer, &rect);

                if (steer_percent != "0") {
                    if (!LoadTextureFromText(steer_percent, textColor))
                        printf("left text failed to load\n");
                    else {
                        rect = {
                            (ww3 / 2) - (textWidth / 2),
                            wh2 - (textHeight / 2),
                            textWidth,
                            textHeight
                        };
                        SDL_RenderCopyEx(renderer, textTexture, NULL, &rect, 0, NULL, SDL_FLIP_NONE);
                    }
                }
            } else if (steer_value_normalized > 0) {
                SDL_Rect rect = {
                    ww3 * 2,
                    0,
                    width,
                    window_height
                };
                SDL_RenderFillRect(renderer, &rect);

                if (steer_percent != "0") {
                    if (!LoadTextureFromText(steer_percent, textColor))
                        printf("right text failed to load\n");
                    else {
                        rect = {
                            window_width - (ww3 / 2) - (textWidth / 2),
                            wh2 - (textHeight / 2),
                            textWidth,
                            textHeight
                        };
                        SDL_RenderCopyEx(renderer, textTexture, NULL, &rect, 0, NULL, SDL_FLIP_NONE);
                    }
                }
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

        // if (!LoadTextureFromText("hello", textColor)) {
        //     printf("text failed to load\n");
        // } else {
        //     SDL_Rect rect = { 20, 20, textWidth, textHeight };
        //     SDL_RenderCopyEx(renderer, textTexture, NULL, &rect, 0, NULL, SDL_FLIP_NONE);
        // }

        SDL_RenderPresent(renderer);
    }

    Close();
    return 0;
}