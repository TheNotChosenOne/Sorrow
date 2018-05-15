#include "rendererSDL.h"

#include <exception>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

#include "utility.h"

namespace {

static inline void setWhite(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

static inline void setColour(SDL_Renderer *renderer, const Vec3 col) {
    SDL_SetRenderDrawColor(renderer,
            static_cast< uint8_t >(clamp(0.0, 255.0, col[0])),
            static_cast< uint8_t >(clamp(0.0, 255.0, col[1])),
            static_cast< uint8_t >(clamp(0.0, 255.0, col[2])),
            0xFF);
}

};

void RendererSDL::drawPoint(Vec pos, Vec3 col) {
    setColour(renderer, col);
    SDL_RenderDrawPoint(renderer, pos[0], height - pos[1] - 1.0);
}

void RendererSDL::drawBox(Vec pos, Vec rad, Vec3 col) {
    setColour(renderer, col);
    SDL_Rect rect;
    rect.w = std::round(rad[0] * 2.0);
    rect.h = std::round(rad[1] * 2.0);
    rect.x = std::round(pos[0] - rad[0]);
    rect.y = height - 1 - (std::round(pos[1] + rad[1]));
    SDL_RenderFillRect(renderer, &rect);
}

void RendererSDL::drawCircle(Vec pos, Vec rad, Vec3 col) {
    setColour(renderer, col);
    const double r = rad[0];

    const int64_t xMin = clamp(0.0, 1.0 * width, std::round(pos[0] - r));
    const int64_t xMax = clamp(0.0, 1.0 * width, std::round(pos[0] + r));
    const int64_t yMin = clamp(0.0, 1.0 * height, std::round(pos[1] - r));
    const int64_t yMax = clamp(0.0, 1.0 * height, std::round(pos[1] + r));

    size_t pixels = 0;
    const double r2 = r * r;
    for (int64_t x = xMin; x <= xMax; ++x) {
        for (int64_t y = yMin; y <= yMax; ++y) {
            const Vec diff = Vec(x, y) - pos;
            if (gmtl::lengthSquared(diff) <= r2) {
                SDL_RenderDrawPoint(renderer, x, height - y);
                ++pixels;
            }
        }
    }
    if (0 == pixels) {
        setColour(renderer, pi< double > * r2 * col);
        SDL_RenderDrawPoint(renderer, std::round(pos[0]), height - std::round(pos[1]));
    }
}

RendererSDL::RendererSDL(size_t width, size_t height)
    : Renderer(width, height)
    , width(width)
    , height(height)
    , window(nullptr)
    , surface(nullptr)
    , renderer(nullptr) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::stringstream ss;
        ss << "Failed to initialize SDL: " << SDL_GetError();
        throw ss.str();
    }

    window = SDL_CreateWindow("Hallway",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            width, height, SDL_WINDOW_SHOWN);

    if (nullptr == window) {
        std::stringstream ss;
        ss << "Failed to create window: " << SDL_GetError();
        throw ss.str();
    }

    surface = SDL_GetWindowSurface(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (nullptr == renderer) {
        std::stringstream ss;
        ss << "Failed to create SDL renderer: " << SDL_GetError();
        throw ss.str();
    }

    clear();
    update();
}

RendererSDL::~RendererSDL() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void RendererSDL::clear() {
    setWhite(renderer);
    SDL_RenderClear(renderer);
}

void RendererSDL::update() {
    SDL_RenderPresent(renderer);
}

size_t RendererSDL::getWidth() const {
    return width;
}

size_t RendererSDL::getHeight() const {
    return height;
}
