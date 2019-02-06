#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <array>
#include <vector>
#include <cassert>

using namespace std;
#define RANDOM_NUM ( rand() / double(RAND_MAX) )

#include "network.hh"
#include "genetics.hh"
#include "math.hh"

const char* WINDOW_TITLE = "✿◡ neatpond ◡✿";
const int HUD_HEIGHT = 160;
const int WINDOW_WIDTH = 720;
const int WINDOW_HEIGHT = 720;

const int WORLD_SIZE = 1000;
const int GENERATION_LIFESPAN = 800;
const int FISH_AMOUNT = 75;
const float FISH_MAX_SPEED = 5.f;
const int FISH_NUM_EYES = 10;
const int FOOD_AMOUNT = 100;
const float FOOD_RESPAWN_RATE = 0.9;
const float FOOD_EAT_DIFFICULTY = 0.0;
const int NN_HIDDEN_LAYERS = 1;
const int NN_HIDDEN_NODES = 1;
const float MUTATION_RATE = .005f;

enum {
  INPUT_SENSOR_FIRST,
  INPUT_SENSOR_LAST = FISH_NUM_EYES - 1,
  INPUT_DIRECTION,
  INPUT_CLOCK_1,
  INPUT_CLOCK_2,
  NUM_INPUTS
};

enum {
  OUTPUT_DIRECTION,
  OUTPUT_SPEED,
  NUM_OUTPUTS
};

enum {
  TRAIT_CLOCK_SPEED,
  TRAIT_CLOCK_SPEED_2,
  TRAIT_FOV,
  TRAIT_RED,
  TRAIT_GREEN,
  TRAIT_BLUE,
  NUM_TRAITS
};

enum {
  SPEED_NORMAL,
  SPEED_FAST,
  SPEED_SONIC,
  NUM_SPEEDS
};

const int DNA_LENGTH =
  NUM_TRAITS + ((NUM_INPUTS + 1) * NN_HIDDEN_NODES) +
  ((NN_HIDDEN_NODES + 1) * NUM_OUTPUTS) +
  ((NN_HIDDEN_NODES + 1) * NN_HIDDEN_NODES * (NN_HIDDEN_LAYERS - 1));

struct GenerationData {
  float averageFitness;
  array<float, 3> averageColor;
};

struct Food {
  Vector2D position;
  bool eaten = false;
};

struct World {
  vector<Food> foods;
};

struct Organism : Genome {
  Network* brain;
  vector<double> sensors;
  vector<double> output;

  Vector2D velocity;
  Vector2D position;

  int foodCollected;
  float fov;
  float angle;
  float sightLength;
  float speed = 0.f;
  float clock = 0.f;

  Organism(DNA genes) : Genome(genes) {
    brain = new Network({NUM_INPUTS, NN_HIDDEN_NODES, NUM_OUTPUTS});
    brain->setWeights(genes);

    fov = genes[TRAIT_FOV] * M_PI;
    sightLength = 300;

    for (int i = NUM_INPUTS; i--;) {
      sensors.push_back(0.0);
    }
  }

  float fitness() const override {
    float foodFitness = foodCollected / float(FOOD_AMOUNT);
    float fitness = powf(foodFitness, 2);
    return fitness;
  }

  void begin() override {
    angle = RANDOM_NUM * M_PI;
    position.x = RANDOM_NUM * WORLD_SIZE;
    position.y = RANDOM_NUM * WORLD_SIZE;
    foodCollected = 0;
    clock = 0.f;
  }

  void eat() {
    if (clock <= GENERATION_LIFESPAN) {
      foodCollected++;
    }
  }

  void update() override {
    brain->feedForward(sensors);
    brain->getResults(output);

    float direction = output[OUTPUT_DIRECTION] > .5f ? 1.f : -1.f;
    float targetSpeed = output[OUTPUT_SPEED] * FISH_MAX_SPEED;

    angle += direction * .1f;
    speed += (targetSpeed - speed) * .1f;
    velocity.x = cosf(angle) * speed;
    velocity.y = sinf(angle) * speed;
    position += velocity;
    clock += 1;

    if (position.x < 0) { position.x = WORLD_SIZE; }
    if (position.y < 0) { position.y = WORLD_SIZE; }
    if (position.x > WORLD_SIZE) { position.x = 0; }
    if (position.y > WORLD_SIZE) { position.y = 0; }
  }

  void perceive(const World& world) {
    for (int i = 0; i < FISH_NUM_EYES; i++) {
      float maxStrength = 0.f;
      float sensorDirection = angle + (-FISH_NUM_EYES / 2 + i) * (fov / float(FISH_NUM_EYES));

      for (auto& food : world.foods) {
        Vector2D line = {
          cosf(sensorDirection) * float(sightLength),
          sinf(sensorDirection) * float(sightLength)
        };
        auto diff = position - food.position;
        float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
        bool collide = lineCircleCollide(position, position + line, food.position, 16);
        if (dist < sightLength && collide) {
          float strength = 1 - dist / float(sightLength);
          if (strength > maxStrength) {
            maxStrength = strength;
          }
        }
      }

      sensors[INPUT_SENSOR_FIRST + i] = maxStrength;
    }

    sensors[INPUT_DIRECTION] = fmod(angle, M_PI * 2) / M_PI;
    sensors[INPUT_CLOCK_1] = fmod(clock * genes[TRAIT_CLOCK_SPEED], 1.0);
    sensors[INPUT_CLOCK_2] = fmod(clock * genes[TRAIT_CLOCK_SPEED_2], (float)GENERATION_LIFESPAN) / (float)GENERATION_LIFESPAN;
  }

};

struct Sprite {
  SDL_Texture* texture;
  SDL_Rect srcRect;
  SDL_Point anchorPoint;

  Sprite(const char *filepath, SDL_Renderer *renderer, float anchorX = .5f, float anchorY = .5f) {
    texture = IMG_LoadTexture(renderer, filepath);
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

int main() {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
  SDL_SetWindowTitle(window, WINDOW_TITLE);

  Sprite sprTail("./res/tail.png", renderer, 1.0, .5);
  Sprite sprBody("./res/body.png", renderer);
  Sprite sprStripes("./res/stripes.png", renderer);
  Sprite sprFood("./res/food.png", renderer);

  World world;
  Population<Organism, FISH_AMOUNT, DNA_LENGTH> population;
  Vector2D camera {0.0, 0.0};
  Evolution evolution;

  vector<GenerationData> generations;
  float maxFitness = 0.0;
  int timePassed = 0;
  bool closed = false;
  bool displayHud = true;
  bool displaySensors = false;
  bool mouseDrag = false;
  int speed = SPEED_NORMAL;

  auto& genomes = population.genomes;

  while (!closed) {
    Uint32 now = SDL_GetTicks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_MOUSEMOTION && mouseDrag) {
        camera.x -= event.motion.xrel;
        camera.y -= event.motion.yrel;
      }
      if (event.type == SDL_MOUSEBUTTONDOWN) { mouseDrag = true;}
      if (event.type == SDL_MOUSEBUTTONUP) { mouseDrag = false; }
      if (event.type == SDL_QUIT) { closed = true; }
      if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          SDL_Rect viewport;
          float s = fmin(event.window.data1, event.window.data2) / (float)WORLD_SIZE;
          SDL_RenderSetScale(renderer, s, s);
        }
      }
      if (event.type == SDL_KEYDOWN) {
        auto& key = event.key.keysym.scancode;
        if (key == SDL_SCANCODE_ESCAPE) {
          closed = true;
        }
        if (key == SDL_SCANCODE_SPACE) {
          speed = (speed + 1) % NUM_SPEEDS;
        }
        if (key == SDL_SCANCODE_V) {
          displaySensors = !displaySensors;
        }
        if (key == SDL_SCANCODE_TAB) {
          displayHud = !displayHud;
        }
      }
    }

    if (speed == SPEED_NORMAL || ++timePassed < GENERATION_LIFESPAN) {
      for (auto& genome : genomes) {
        genome.perceive(world);
        genome.update();

        for (auto& food : world.foods) {
          float mouthX = genome.position.x + cosf(genome.angle) * 8.f;
          float mouthY = genome.position.y + sinf(genome.angle) * 8.f;
          float distX = mouthX - food.position.x;
          float distY = mouthY - food.position.y;
          float distance = sqrt(distX * distX + distY * distY);
          if (distance <= 16 && bool(RANDOM_NUM > FOOD_EAT_DIFFICULTY)) {
            food.eaten = bool(RANDOM_NUM > FOOD_RESPAWN_RATE);
            food.position.x = RANDOM_NUM * WORLD_SIZE;
            food.position.y = RANDOM_NUM * WORLD_SIZE;
            genome.eat();
          }
        }
      }
      world.foods.erase(
        remove_if(begin(world.foods), end(world.foods),
        [](Food& food) {
          return food.eaten;
        }),
        end(world.foods));
    } else {
      auto averageFitness = evolution.reproduce<Organism>(genomes, MUTATION_RATE);
      float aR = 0.0;
      float aG = 0.0;
      float aB = 0.0;

      for (auto& g : genomes) {
        aR += g.genes[TRAIT_RED];
        aG += g.genes[TRAIT_GREEN];
        aB += g.genes[TRAIT_BLUE];
      }

      aR /= float(genomes.size());
      aG /= float(genomes.size());
      aB /= float(genomes.size());

      maxFitness = fmax(maxFitness, averageFitness);

      generations.push_back({
        averageFitness,
        {aR, aG, aB}
      });

      cout <<
        "Generation: " << generations.size() << "\n" <<
        "Top: " << maxFitness << "\n" <<
        "Average: " << averageFitness << "\n" <<
      endl;

      timePassed = 0;

      for (auto& genome : genomes) {
        genome.begin();
      }

      world.foods.clear();
      for (auto i(0u); i < FOOD_AMOUNT; i++) {
        Food food;
        food.position.x = RANDOM_NUM * WORLD_SIZE;
        food.position.y = RANDOM_NUM * WORLD_SIZE;
        world.foods.push_back(food);
      }
    }

    if (speed != SPEED_SONIC || timePassed == 0) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);

      SDL_Rect background;
      background.x = -camera.x;
      background.y = -camera.y;
      background.w = WORLD_SIZE;
      background.h = WORLD_SIZE;

      SDL_SetRenderDrawColor(renderer, 26, 33, 44, 255);
      SDL_RenderFillRect(renderer, &background);

      int i = 0;
      for (auto& genome : genomes) {
        auto genes = genome.genes;
        float angle = genome.angle;
        float tailAngle = sin((float)timePassed / 2) * 0.25;
        float bodyAngle = sin((float)timePassed / 10) * 0.2;
        float x = genome.position.x - camera.x;
        float y = genome.position.y - camera.y;
        int r = genes[TRAIT_RED] * 255;
        int g = genes[TRAIT_GREEN] * 255;
        int b = genes[TRAIT_BLUE] * 255;

        // draw sensors
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        if (displaySensors && genome.sensors.size() >= FISH_NUM_EYES) {
          for (int i = 0; i < FISH_NUM_EYES; i++) {
            float strength = genome.sensors[i];
            float sensorDirection = genome.angle + (-FISH_NUM_EYES / 2 + i) * (genome.fov / (float)FISH_NUM_EYES);
            float x2 = x + cosf(sensorDirection) * 60;
            float y2 = y + sinf(sensorDirection) * 60;
            SDL_SetRenderDrawColor(renderer, strength * 255, strength * 255, strength * 255, 255);
            SDL_RenderDrawLine(renderer, x, y, x2, y2);
          }
        }

        // draw fish
        sprTail.draw(renderer, x - cos(angle) * 12, y - sin(angle) * 12, angle + tailAngle, r, g, b);
        sprBody.draw(renderer, x, y, angle);
        sprStripes.draw(renderer, x, y, angle, r, g, b);
        i++;
      }

      for (auto& food : world.foods) {
        sprFood.draw(renderer, food.position.x - camera.x, food.position.y - camera.y);
      }

      // draw chart
      if (displayHud) {
        int chartTop = WINDOW_HEIGHT - HUD_HEIGHT;
        int chartMargin = 8;
        int chartWidth = WINDOW_WIDTH;
        int chartHeight = HUD_HEIGHT;
        int numData = 200;
        int barMargin = chartMargin + 2;
        float barWidth = float(chartWidth - barMargin * 2) / float(numData);

        SDL_Rect chart = {
          chartMargin,
          chartTop + chartMargin,
          chartWidth - chartMargin * 2,
          chartHeight - chartMargin * 2
        };
        SDL_SetRenderDrawColor(renderer, 255, 250, 244, 255);
        SDL_RenderFillRect(renderer, &chart);

        int offset = max(0, int(generations.size() - numData));
        int length = min(numData, int(generations.size()));

        vector<GenerationData> subData(
          generations.cbegin() + offset,
          generations.cbegin() + offset + length
        );

        for (int i = 0; i < subData.size(); i++) {
          auto fitness = subData[i].averageFitness;
          auto color = subData[i].averageColor;
          float value = fitness / maxFitness;
          int r = color[0] * 255;
          int g = color[1] * 255;
          int b = color[2] * 255;
          int bar_height = chartHeight * value;
          SDL_Rect bar;
          bar.x = float(barWidth * float(i) + float(barMargin));
          bar.y = chartTop + chartHeight - bar_height + barMargin;
          bar.w = barWidth;
          bar.h = max(0, bar_height - barMargin * 2);
          SDL_SetRenderDrawColor(renderer, r, g, b, 255);
          SDL_RenderFillRect(renderer, &bar);
        }
      }

      SDL_RenderPresent(renderer);

      if (speed == SPEED_NORMAL) {
        SDL_Delay(1000 / 60);
      }
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
