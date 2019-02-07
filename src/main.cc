#include <SDL2/SDL.h>

#include "math.hh"
#include "network.hh"
#include "genetics.hh"
#include "graphics.hh"
#include "pond.hh"

using namespace std;

const char* WINDOW_TITLE = "✿◡ neatpond ◡✿";
int windowWidth = 720;
int windowHeight = 720;

int main() {
  srand(time(NULL));

  SDL_Init(SDL_INIT_EVERYTHING);

  Renderer renderer(
    WINDOW_TITLE,
    windowWidth,
    windowHeight,
  {
    {"tail", "res/tail.png", 1.0, 0.5},
    {"body", "res/body.png"},
    {"stripes", "res/stripes.png"},
    {"food", "res/food.png"},
    {"ded", "res/ded.png"},
  });

  NeatPond pond;
  int speed = SPEED_NORMAL;
  int numGenerations = 0;
  int generationTime = 0;
  float maxFitness = 0.0;
  vector<float> averageFitnesses;
  vector<array<float, 3>> averageColors;

  bool closed = false;
  bool displayHud = true;
  auto timeStart = SDL_GetTicks();
  Vector2D camera;
  Vector2D mouse;
  bool mouseDrag = false;
  bool mouseDiscardClick = false;
  int selectedGenome = -1;

  while (!closed) {
    auto now = SDL_GetTicks();
    SDL_Event event;

    auto& genomes = pond.getGenomes();
    auto& foods = pond.getFood();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        closed = true;
      }

      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
        windowWidth = event.window.data1;
        windowHeight = event.window.data2;
        renderer.resize(windowWidth, windowHeight);
      }

      if (event.type == SDL_MOUSEMOTION) {
        int xrel = event.motion.xrel;
        int yrel = event.motion.yrel;
        if (mouseDrag) {
          camera.x = fmin(WORLD_SIZE - windowWidth, fmax(camera.x - xrel, 0));
          camera.y = fmin(WORLD_SIZE - windowHeight, fmax(camera.y - yrel, 0));
          if (abs(xrel) > 1 || abs(yrel) > 1) {
            mouseDiscardClick = true;
          }
        }
        mouse.x = event.motion.x;
        mouse.y = event.motion.y;
      }

      if (event.type == SDL_MOUSEBUTTONDOWN) {
        mouseDrag = true;
        mouseDiscardClick = false;
      }

      if (event.type == SDL_MOUSEBUTTONUP) {
        mouseDrag = false;
        if (!mouseDiscardClick) {
          selectedGenome = -1;
          for (int i = genomes.size(); i--;) {
            auto& genome = genomes[i];
            auto dist = genome.position - (mouse + camera);
            if (fabs(dist.x) < 32 && fabs(dist.y) < 32) {
              selectedGenome = i;
            }
          }
        }
      }

      if (event.type == SDL_KEYDOWN) {
        auto& key = event.key.keysym.scancode;

        if (key == SDL_SCANCODE_ESCAPE) {
          closed = true;
        }
        if (key == SDL_SCANCODE_F) {
          pond.spawnFood(mouse + camera);
        }
        if (key == SDL_SCANCODE_SPACE) {
          speed = (speed + 1) % NUM_SPEEDS;
        }
        if (key == SDL_SCANCODE_TAB) {
          displayHud = !displayHud;
        }
      }
    }

    pond.update();

    if (speed != SPEED_NORMAL && ++generationTime >= GENERATION_LIFESPAN) {
      auto averageFitness = pond.reset();
      auto numGenomes = genomes.size();

      float r = 0.0;
      float g = 0.0;
      float b = 0.0;

      for (auto& genome : genomes) {
        r += genome.genes[TRAIT_RED];
        g += genome.genes[TRAIT_GREEN];
        b += genome.genes[TRAIT_BLUE];
      }

      r /= float(numGenomes);
      g /= float(numGenomes);
      b /= float(numGenomes);

      maxFitness = fmax(maxFitness, averageFitness);
      averageFitnesses.push_back(averageFitness);
      averageColors.push_back({r, g, b});

      int seconds = (now - timeStart) / 1000;
      cout <<
        "Generation: " << numGenerations << "\n" <<
        "  Minutes: " << ((float)seconds / 60.0) << "\n" <<
        "  Top: " << maxFitness << "\n" <<
        "  Average: " << averageFitness << "\n" <<
      endl;

      generationTime = 0;
      numGenerations++;
    }

    if (speed != SPEED_SONIC || generationTime == 0) {
      renderer.color(0, 0, 0);
      renderer.clear();

      SDL_Rect background;
      background.x = -camera.x;
      background.y = -camera.y;
      background.w = WORLD_SIZE;
      background.h = WORLD_SIZE;

      for (int i = genomes.size(); i--;) {
        auto &genome = genomes[i];
        auto genes = genome.genes;
        int r = genes[TRAIT_RED] * 255;
        int g = genes[TRAIT_GREEN] * 255;
        int b = genes[TRAIT_BLUE] * 255;
        float x = genome.position.x - camera.x;
        float y = genome.position.y - camera.y;
        float tailAngle = sin((float)generationTime / 2) * 0.25;
        float bodyAngle = sin((float)generationTime / 10) * 0.2;

        if (i == selectedGenome) {
          for (int i = 0; i < FISH_NUM_EYES; i++) {
            float strength = genome.input[i];
            float sensorDirection = genome.angle + (-FISH_NUM_EYES / 2 + i) * (genome.fov / (float)FISH_NUM_EYES);
            float r = strength > .5 ? 1 - 2 * (strength - .5) : 1.0;
            float g = strength > .5 ? 1 : 2 * strength;
            float x2 = x + cosf(sensorDirection) * 60;
            float y2 = y + sinf(sensorDirection) * 60;
            renderer.color(r * 255, g * 255, 125);
            renderer.drawLine(x, y, x2, y2);
          }
        }

        if (genome.energy > 0.0) {
          renderer.drawSprite("tail", x - cos(genome.angle) * 12, y - sin(genome.angle) * 12, genome.angle + tailAngle, r, g, b);
          renderer.drawSprite("body", x, y, genome.angle);
          renderer.drawSprite("stripes", x, y, genome.angle, r, g, b);
        } else {
          renderer.drawSprite("ded", x, y);
        }
      }

      for (auto& food : foods) {
        renderer.drawSprite("food", food.position.x - camera.x, food.position.y - camera.y);
      }

      if (displayHud) {
        if (selectedGenome >= 0 && selectedGenome < genomes.size()) {
          renderer.drawNetwork(*genomes[selectedGenome].brain);
        }
        renderer.drawChart(averageFitnesses, averageColors, maxFitness);
      }

      renderer.present();

      if (speed == SPEED_NORMAL) {
        SDL_Delay(1000 / 60);
      }
    }
  }

  SDL_Quit();

  return 0;
}
