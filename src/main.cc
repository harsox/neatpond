#include <SDL2/SDL.h>
#include <iostream>
#include <array>
#include <vector>
#include <cassert>

using namespace std;
#define RANDOM_NUM ( rand() / double(RAND_MAX) )

#include "graphics.hh"
#include "math.hh"
#include "network.hh"
#include "genetics.hh"

const int WORLD_SIZE = 1500;
const int GENERATION_LIFESPAN = 800;
const int FISH_AMOUNT = 100;
const float FISH_MAX_SPEED = 5.f;
const int FISH_NUM_EYES = 8;
const int FOOD_AMOUNT = 200;
const float FOOD_RESPAWN_RATE = 0.05;
const float FOOD_EAT_DIFFICULTY = 0.0;
const float MUTATION_RATE = .005f;
const int HIDDEN_LAYERS = 1;
const int HIDDEN_NODES = 2;

enum {
  INPUT_SENSOR_FIRST,
  INPUT_SENSOR_LAST = FISH_NUM_EYES - 1,
  INPUT_DIRECTION,
  INPUT_SPEED,
  INPUT_ENERGY,
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
  NUM_TRAITS + ((NUM_INPUTS + 1) * HIDDEN_NODES) +
  ((HIDDEN_NODES + 1) * NUM_OUTPUTS) +
  ((HIDDEN_NODES + 1) * HIDDEN_NODES * (HIDDEN_LAYERS - 1));

struct Food {
  Vector2D position;
  bool eaten = false;
};

struct World {
  vector<Food> foods;
};

struct Organism : Genome {
  Network* brain;
  vector<double> input;
  vector<double> output;

  Vector2D velocity;
  Vector2D position;

  int foodCollected;
  float fov;
  float angle;
  float sightLength;
  float speed = 0.f;
  float clock = 0.f;
  float energy = 1000.f;

  Organism(DNA genes) : Genome(genes) {
    // skip trait genes
    vector<double> weightGenes(
      genes.cbegin() + NUM_TRAITS,
      genes.cend()
    );

    brain = new Network({NUM_INPUTS, HIDDEN_NODES, NUM_OUTPUTS});
    brain->setWeights(weightGenes);

    fov = genes[TRAIT_FOV] * M_PI;
    sightLength = 300;

    for (int i = NUM_INPUTS; i--;) {
      input.push_back(0.0);
    }
  }

  float fitness() const override {
    // if (energy <= 0.0) { return 0; }
    float foodFitness = foodCollected / float(FOOD_AMOUNT);
    float fitness = powf(foodFitness, 2);
    return fitness;
  }

  void begin() override {
    angle = RANDOM_NUM * M_PI * 2;
    position.x = RANDOM_NUM * WORLD_SIZE;
    position.y = RANDOM_NUM * WORLD_SIZE;
    foodCollected = 0;
    clock = 0.f;
  }

  void eat() {
    if (energy > 0.0 && clock <= GENERATION_LIFESPAN) {
      foodCollected++;
      energy += 100.0;
    }
  }

  void update() override {
    if (energy <= 0.0) { return; }
    brain->feedForward(input);
    brain->getResults(output);

    float turnSpeed = output[OUTPUT_DIRECTION] * 2.0 -1.0;
    float targetSpeed = output[OUTPUT_SPEED] * FISH_MAX_SPEED;
    float acc = targetSpeed > speed ? 1 : 0.01;

    angle += turnSpeed * .2;
    speed += (targetSpeed - speed) * acc;

    if (clock <= GENERATION_LIFESPAN) {
      energy -= powf(targetSpeed * 0.25, 2);
    }

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
    if (energy <= 0.0) { return; }
    for (int i = 0; i < FISH_NUM_EYES; i++) {
      float maxStrength = 0.f;
      float sensorDirection = angle + (-FISH_NUM_EYES / 2 + i) * (fov / float(FISH_NUM_EYES));
      for (auto& food : world.foods) {
        Vector2D line = {
          cosf(sensorDirection) * float(sightLength),
          sinf(sensorDirection) * float(sightLength)
        };
        auto diff = position - food.position;
        if (fabs(diff.x) < sightLength && fabs(diff.y) < sightLength) {
          float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
          if (dist < sightLength && lineCircleCollide(position, position + line, food.position, 16)) {
            float strength = 1 - dist / float(sightLength);
            if (strength > maxStrength) {
              maxStrength = strength;
            }
          }
        }
      }
      input[INPUT_SENSOR_FIRST + i] = maxStrength;
    }

    input[INPUT_DIRECTION] = modAngle(angle) / (M_PI * 2);
    input[INPUT_SPEED] = speed / FISH_MAX_SPEED;
    input[INPUT_ENERGY] = fmax(energy / 1000.0, 1.0);
    input[INPUT_CLOCK_1] = fmod(clock * genes[TRAIT_CLOCK_SPEED], 1.0);
    input[INPUT_CLOCK_2] = fmod(clock * genes[TRAIT_CLOCK_SPEED_2], (float)GENERATION_LIFESPAN) / (float)GENERATION_LIFESPAN;
  }
};

int main() {
  SDL_Init(SDL_INIT_EVERYTHING);

  Renderer renderer({
    {"tail", "res/tail.png", 1.0, 0.5},
    {"body", "res/body.png"},
    {"stripes", "res/stripes.png"},
    {"food", "res/food.png"},
    {"ded", "res/ded.png"},
  });

  World world;
  Population<Organism, FISH_AMOUNT, DNA_LENGTH> population;
  Vector2D camera {0.0, 0.0};
  Evolution evolution;

  vector<GenerationData> generations;
  float maxFitness = 0.0;
  int timePassed = 0;
  int speed = SPEED_NORMAL;
  int selectedGenome = -1;
  bool closed = false;
  bool displayHud = true;
  auto timeStart = SDL_GetTicks();
  auto& genomes = population.genomes;

  Vector2D mouse;
  bool mouseDrag = false;
  bool mouseDiscardClick = false;

  while (!closed) {
    auto now = SDL_GetTicks();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_MOUSEMOTION) {
        int xrel = event.motion.xrel;
        int yrel = event.motion.yrel;
        if (mouseDrag) {
          camera.x = fmin(WORLD_SIZE - WINDOW_WIDTH, fmax(camera.x - xrel, 0));
          camera.y = fmin(WORLD_SIZE - WINDOW_HEIGHT, fmax(camera.y - yrel, 0));
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
      if (event.type == SDL_QUIT) { closed = true; }
      if (event.type == SDL_KEYDOWN) {
        auto& key = event.key.keysym.scancode;
        if (key == SDL_SCANCODE_ESCAPE) {
          closed = true;
        }
        if (key == SDL_SCANCODE_SPACE) {
          speed = (speed + 1) % NUM_SPEEDS;
        }
        if (key == SDL_SCANCODE_F) {
          world.foods.push_back({mouse + camera});
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
            food.eaten = bool(RANDOM_NUM < FOOD_RESPAWN_RATE);
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
      timePassed = 0;

      generations.push_back({
        averageFitness,
        {aR, aG, aB}
      });

      int seconds = (now - timeStart) / 1000;
      cout <<
        "Generation: " << generations.size() << "\n" <<
        "  Minutes: " << ((float)seconds / 60.0) << "\n" <<
        "  Top: " << maxFitness << "\n" <<
        "  Average: " << averageFitness << "\n" <<
      endl;

      for (auto& genome : genomes) {
        genome.begin();
      }

      world.foods.clear();
      for (int i = FOOD_AMOUNT; i--;) {
        Food food;
        food.position.x = RANDOM_NUM * WORLD_SIZE;
        food.position.y = RANDOM_NUM * WORLD_SIZE;
        world.foods.push_back(food);
      }
    }

    if (speed != SPEED_SONIC || timePassed == 0) {
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
        float angle = modAngle(genome.angle);
        float tailAngle = sin((float)timePassed / 2) * 0.25;
        float bodyAngle = sin((float)timePassed / 10) * 0.2;

        // draw sensors
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

        // draw fish
        if (genome.energy > 0.0) {
          renderer.drawSprite("tail", x - cos(angle) * 12, y - sin(angle) * 12, angle + tailAngle, r, g, b);
          renderer.drawSprite("body", x, y, angle);
          renderer.drawSprite("stripes", x, y, angle, r, g, b);
        } else {
          renderer.drawSprite("ded", x, y);
        }
      }

      for (auto& food : world.foods) {
        renderer.drawSprite("food", food.position.x - camera.x, food.position.y - camera.y);
      }

      if (displayHud) {
        if (selectedGenome >= 0 && selectedGenome < genomes.size()) {
          renderer.drawNetwork(*genomes[selectedGenome].brain);
        }
        renderer.drawChart(generations, maxFitness);
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
