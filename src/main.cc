#include "math.hh"
#include "network.hh"
#include "genetics.hh"
#include "graphics.hh"
#include "pond.hh"

#include <SDL2/SDL.h>

using namespace std;

const char* WINDOW_TITLE = "✿◡ neatpond ◡✿";
int windowWidth = 960;
int windowHeight = 720;

void runHeadless() {
  int t = 0;
  int g = 0;

  NeatPond pond;

  while (true) {
    pond.update();
    if (++t > GENERATION_LIFESPAN) {
      float f = pond.reset();
      cout << "generation: " << g << endl;
      cout << "fitness: " << f << endl;
      t = 0;
      g++;
    }
  }
}

void runGUI() {
  SDL_Init(SDL_INIT_EVERYTHING);

  Renderer renderer(WINDOW_TITLE, windowWidth, windowHeight);
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
  Vector2D camera((WORLD_SIZE - windowWidth) / 2, (WORLD_SIZE - windowHeight) / 2);
  Vector2D mouse;
  const Vector2D* followPosition = nullptr;
  bool mouseDrag = false;
  bool mouseDiscardClick = false;
  int selectedFish = -1;

  while (!closed) {
    auto now = SDL_GetTicks();
    SDL_Event event;

    auto& fishes = pond.getFishes();

    if (followPosition != nullptr) {
      camera.x = followPosition->x - windowWidth / 2;
      camera.y = followPosition->y - windowHeight / 2;
    }

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
            followPosition = nullptr;
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
          selectedFish = -1;
          followPosition = nullptr;
          for (int i = fishes.size(); i--;) {
            auto& genome = fishes[i];
            auto dist = genome.position - (mouse + camera);
            if (fabs(dist.x) < 80 && fabs(dist.y) < 80) {
              selectedFish = i;
              followPosition = &genome.position;
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
      auto numFishes = fishes.size();

      float r = 0.0;
      float g = 0.0;
      float b = 0.0;

      for (auto& genome : fishes) {
        r += genome.genes[TRAIT_RED];
        g += genome.genes[TRAIT_GREEN];
        b += genome.genes[TRAIT_BLUE];
      }

      r /= float(numFishes);
      g /= float(numFishes);
      b /= float(numFishes);

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

      renderer.translate(-camera.x, -camera.y);

      int chunkSize = GRID_SIZE;
      for (int x = 0; x < WORLD_CHUNKS; x++) {
        for (int y = 0; y < WORLD_CHUNKS; y++) {
          if ((x + y) % 2 == 0) {
            renderer.color(3, 5, 25);
            renderer.rect(
              x * chunkSize,
              y * chunkSize,
              chunkSize,
              chunkSize
            );
          }
        }
      }

      renderer.drawPond(pond, selectedFish);

      renderer.translate(0, 0);
      if (displayHud) {
        if (selectedFish >= 0 && selectedFish < fishes.size()) {
          renderer.drawNetwork(fishes[selectedFish].brain);
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
}

int main(int argc, char **argv) {
  srand(time(NULL));
  if (argc > 1 && strcmp(argv[1], "-headless") == 0) {
    runHeadless();
  } else {
    runGUI();
  }
  return 0;
}
