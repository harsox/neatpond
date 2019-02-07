#ifndef graphics_h
#define graphics_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <algorithm>
#include <array>
#include <map>

#include "network.hh"

const char* WINDOW_TITLE = "✿◡ neatpond ◡✿";
const int WINDOW_WIDTH = 720;
const int WINDOW_HEIGHT = 720;
const int HUD_HEIGHT = 100;

struct GenerationData {
  float averageFitness;
  array<float, 3> averageColor;
};

struct SpriteSource {
  string identifier;
  string source;
  float anchorX = 0.5;
  float anchorY = 0.5;
};

struct Sprite {
  SDL_Texture* texture;
  SDL_Rect srcRect;
  SDL_Point anchorPoint;

  Sprite(SDL_Renderer *renderer, const SpriteSource& spr) {
    texture = IMG_LoadTexture(renderer, spr.source.c_str());
    assert(texture != 0x0);
    SDL_QueryTexture(texture, nullptr, nullptr, &srcRect.w, &srcRect.h);
    assert(srcRect.w && srcRect.h);
    srcRect.x = 0;
    srcRect.y = 0;
    anchorPoint.x = srcRect.w * spr.anchorX;
    anchorPoint.y = srcRect.h * spr.anchorY;
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
public:
  ~Renderer() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
  }

  Renderer(vector<SpriteSource> _sprites) {
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetWindowTitle(window, WINDOW_TITLE);

    for (auto& s : _sprites) {
      auto* sprite = new Sprite(renderer, s);
      sprites.insert(make_pair(s.identifier, sprite));
    }
  }

  void drawSprite(const string& id, int x, int y, float angle = .0f, int r = 255, int g = 255, int b = 255) {
    sprites[id]->draw(renderer, x, y, angle, r, g, b);
  }

  void color(int r, int g, int b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
  }

  void clear() {
    SDL_RenderClear(renderer);
  }

  void drawChart(const vector<GenerationData>& data, float maxFitness) {
    int chartTop = WINDOW_HEIGHT - HUD_HEIGHT;
    int chartWidth = WINDOW_WIDTH / 2;
    int chartHeight = HUD_HEIGHT;
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

    int offset = max(0, int(data.size() - dataChunkSize));
    int length = min(dataChunkSize, int(data.size()));

    vector<GenerationData> subData(
      data.cbegin() + offset,
      data.cbegin() + offset + length
    );

    for (int i = 0; i < subData.size(); i++) {
      auto fitness = subData[i].averageFitness;
      auto color = subData[i].averageColor;
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

  void drawLine(int x1, int y1, int x2, int y2) {
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
  }

  void drawNetwork(Network& net) {
    vector<Layer> layers = net.getLayers();
    int numLayers = layers.size();

    int graphWidth = WINDOW_WIDTH / 2;
    int graphHeight = 120;
    int nodeSize = 8;
    int nodeSize_2 = 4;
    int nodeSpacing = nodeSize * 1.75;
    int layerSpacing = graphWidth / (numLayers + 1);
    int xOffset = WINDOW_WIDTH / 2;//96 + (graphWidth - numLayers * layerSpacing) / 2;
    int yOffset = WINDOW_HEIGHT - graphHeight;

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

  void present() {
    SDL_RenderPresent(renderer);
  }

private:
  SDL_Window* window;
  SDL_Renderer* renderer;
  map<string, Sprite*> sprites;
};


#endif