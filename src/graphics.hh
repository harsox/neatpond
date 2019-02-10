#ifndef graphics_h
#define graphics_h

#include "network.hh"
#include "pond.hh"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <array>

enum {
  SPRITE_FISH_BODY,
  SPRITE_FISH_TAIL,
  SPRITE_FISH_FIN,
  SPRITE_FOOD,
  SPRITE_DEAD,
  NUM_SPRITE
};

const int HUD_HEIGHT = 100;

struct Sprite {
  SDL_Texture* texture;
  SDL_Rect srcRect;
  SDL_Point anchorPoint;

  Sprite(SDL_Renderer *renderer, const char* source, float anchorX = 0.5, float anchorY = 0.5) {
    texture = IMG_LoadTexture(renderer, source);
    assert(texture != 0x0);
    SDL_QueryTexture(texture, nullptr, nullptr, &srcRect.w, &srcRect.h);
    assert(srcRect.w && srcRect.h);
    srcRect.x = 0;
    srcRect.y = 0;
    anchorPoint.x = srcRect.w * anchorX;
    anchorPoint.y = srcRect.h * anchorY;
  }

  void draw(SDL_Renderer *renderer, int x, int y, float angle = .0f, int r = 255, int g = 255, int b = 255) {
    SDL_Rect destRect {
      x - anchorPoint.x,
      y - anchorPoint.y,
      srcRect.w, srcRect.h,
    };
    if (r != 255 || g != 255 || b != 255) {
      SDL_SetTextureColorMod(texture, r, g, b);
    }
    SDL_RenderCopyEx(renderer, texture, &srcRect, &destRect, angle * (180 / M_PI), &anchorPoint, SDL_FLIP_NONE);
  }
};

void drawNeuron(SDL_Renderer* renderer, Neuron& neuron, int x, int y, int size) {
  float output = neuron.getOutput();
  float r = output > .5 ? 1 - 2 * (output - .5) : 1.0;
  float g = output > .5 ? 1 : 2 * output;
  SDL_Rect outlineRect, innerRect;
  outlineRect.x = x - 2;
  outlineRect.y = y - 2;
  outlineRect.w = size + 4;
  outlineRect.h = size + 4;
  innerRect.x = x + size * 0.5 * (1 - output);
  innerRect.y = y + size * 0.5 * (1 - output);
  innerRect.w = size * fmax(0, fmin(1, output));
  innerRect.h = size * fmax(0, fmin(1, output));
  SDL_SetRenderDrawColor(renderer, 255 * r, 255 * g, 125, 255);
  SDL_RenderDrawRect(renderer, &outlineRect);
  SDL_RenderFillRect(renderer, &innerRect);
}

class Renderer {
private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  Sprite* sprites[NUM_SPRITE];
  int windowWidth;
  int windowHeight;

public:
  ~Renderer() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
  }

  Renderer(const char* title, int w, int h) {
    SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetWindowTitle(window, title);
    windowWidth = w;
    windowHeight = h;
    sprites[SPRITE_FISH_BODY] = new Sprite(renderer, "res/body.png");
    sprites[SPRITE_FISH_TAIL] = new Sprite(renderer, "res/tail.png", 1.0, 0.5);
    sprites[SPRITE_FISH_FIN] = new Sprite(renderer, "res/fin.png");
    sprites[SPRITE_FOOD] = new Sprite(renderer, "res/food.png");
    sprites[SPRITE_DEAD] = new Sprite(renderer, "res/dead.png");
  }

  void resize(int w, int h) {
    windowWidth = w;
    windowHeight = h;
  }

  void translate(int x, int y) {
    SDL_Rect view;
    view.x = x;
    view.y = y;
    view.w = windowWidth - x;
    view.h = windowHeight - y;
    SDL_RenderSetViewport(renderer, &view);
  }

  void drawSprite(size_t spriteId, int x, int y, float angle = .0f, int r = 255, int g = 255, int b = 255) {
    sprites[spriteId]->draw(renderer, x, y, angle, r, g, b);
  }

  void rect(int x, int y, int w, int h) {
    SDL_Rect rect { x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
  }

  void drawLine(int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
  }

  void color(int r, int g, int b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
  }

  void clear() {
    SDL_RenderClear(renderer);
  }

  void present() {
    SDL_RenderPresent(renderer);
  }

  void drawPond(const NeatPond& pond, int selectedFish = -1) {
    auto& fishes = pond.getFishes();
    auto& foods = pond.getFood();

    for (int i = fishes.size(); i--;) {
      drawFish(fishes[i], i == selectedFish);
    }

    for (auto& food : foods) {
      if (selectedFish == -1 || fishes[selectedFish].canSeeFood(food)) {
        drawSprite(SPRITE_FOOD, food.position.x, food.position.y);
      }
    }
  }

  void drawFish(const Fish& fish, bool drawSensors) {
    auto genes = fish.genes;
    int r = genes[TRAIT_RED] * 255;
    int g = genes[TRAIT_GREEN] * 255;
    int b = genes[TRAIT_BLUE] * 255;
    float x = fish.position.x;
    float y = fish.position.y;
    float tailAngle = sin((float)0.0 / 2) * 0.25;
    float bodyAngle = sin((float)0.0 / 10) * 0.2;

    if (drawSensors) {
      for (int i = 0; i < FISH_NUM_EYES; i++) {
        float strength = fish.input[i];
        float sensorDirection = fish.angle + (-FISH_NUM_EYES / 2 + i) * (fish.fov / (float)FISH_NUM_EYES);
        float r = strength > .5 ? 1 - 2 * (strength - .5) : 1.0;
        float g = strength > .5 ? 1 : 2 * strength;
        float x2 = x + cosf(sensorDirection) * 60;
        float y2 = y + sinf(sensorDirection) * 60;
        color(r * 255, g * 255, 125);
        drawLine(x, y, x2, y2);
      }
    }

    if (!fish.dead) {
      drawSprite(SPRITE_FISH_TAIL, x - cos(fish.angle) * 12, y - sin(fish.angle) * 12, fish.angle + tailAngle, r, g, b);
      drawSprite(SPRITE_FISH_BODY, x, y, fish.angle);
      drawSprite(SPRITE_FISH_FIN, x, y, fish.angle, r, g, b);
    } else {
      drawSprite(SPRITE_DEAD, x, y);
    }
  }

  void drawChart(
    const vector<float>& averageFitnesses,
    const vector<array<float, 3>>& averageColors,
    float maxFitness
  ) {
    assert(averageFitnesses.size() == averageColors.size());

    int chartWidth = windowWidth / 2;
    int chartHeight = HUD_HEIGHT;
    int chartTop = windowHeight - HUD_HEIGHT;
    int chartMargin = 8;
    int dataChunkSize = 100;
    int barMargin = chartMargin + 2;
    float barWidth = float(chartWidth - barMargin * 2) / float(dataChunkSize);
    SDL_Rect chart = {
      chartMargin,
      chartTop + chartMargin,
      chartWidth - chartMargin * 2,
      chartHeight - chartMargin * 2
    };

    SDL_SetRenderDrawColor(renderer, 255, 250, 244, 255);
    SDL_RenderFillRect(renderer, &chart);

    int offset = max(0, int(averageFitnesses.size() - dataChunkSize));
    int length = min(dataChunkSize, int(averageFitnesses.size()));

    for (int i = 0; i < length; i++) {
      auto fitness = averageFitnesses[offset + i];
      auto color = averageColors[offset + i];
      float value = fitness / (maxFitness > 0 ? maxFitness : 1);
      int barHeight = chartHeight * value;
      int r = color[0] * 255;
      int g = color[1] * 255;
      int b = color[2] * 255;
      SDL_Rect bar;
      bar.x = float(barWidth * float(i) + float(barMargin));
      bar.y = chartTop + chartHeight - barHeight + barMargin;
      bar.w = barWidth;
      bar.h = max(0, barHeight - barMargin * 2);
      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_RenderFillRect(renderer, &bar);
    }
  }

  void drawNetwork(const Network& net) {
    vector<Layer> layers = net.getLayers();
    int numLayers = layers.size();

    int graphWidth = 250;
    int graphHeight = 120;
    int nodeSize = 8;
    int nodeSize_2 = 4;
    int nodeSpacing = nodeSize * 1.75;
    int layerSpacing = graphWidth / (numLayers + 1);
    int xOffset = (graphWidth - numLayers * layerSpacing) / 2;
    int yOffset = 120;

    for (int l = 0; l < numLayers; l++ ) {
      Layer &layer = layers[l];
      int numNeurons = layer.size();
      int x = xOffset + l * layerSpacing;
      for (int n = 0; n < numNeurons; n++) {
        int y = yOffset + (-(float)numNeurons / 2 + n) * nodeSpacing;
        Neuron &neuron = layer[n];
        vector<double> connections = neuron.getConnectionWeights();
        drawNeuron(renderer, neuron, x, y, nodeSize);
        for (int c = 0; c < connections.size(); c++) {
          int numConnectedNeurons = layers[l + 1].size();
          for (int n2 = 0; n2 < numConnectedNeurons - 1; n2++) {
            int y2 = yOffset + (-(float)numConnectedNeurons / 2 + n2) * nodeSpacing;
            float weight = 0.5 + connections[c] / 2;
            float r = weight > .5 ? 1 - 2 * (weight - .5) : 1.0;
            float g = weight > .5 ? 1 : 2 * weight;
            SDL_SetRenderDrawColor(renderer, 255 * r, 255 * g, 125, 255);
            SDL_RenderDrawLine(renderer, x + nodeSize_2, y + nodeSize_2, x + layerSpacing + nodeSize_2, y2 + nodeSize_2);
          }
        }
      }
    }
  }
};


#endif
