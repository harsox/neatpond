#ifndef pond_h
#define pond_h

#include <vector>

#include "utils.hh"
#include "math.hh"
#include "genetics.hh"
#include "network.hh"

using namespace std;

const int WORLD_SIZE = 1000;
const int GRID_SIZE = 10;
const int GENERATION_LIFESPAN = 800;
const int FISH_AMOUNT = 100;
const float FISH_MAX_SPEED = 5.0;
const int FISH_NUM_EYES = 10;
const int FOOD_AMOUNT = 100;
const float FOOD_RESPAWN_RATE = 0.05;
const float FOOD_EAT_DIFFICULTY = 0.0;
const float MUTATION_RATE = 0.005;
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

int getMapGrid(float x, float y) {
  int xx = floor(x / (WORLD_SIZE / GRID_SIZE));
  int yy = floor(y / (WORLD_SIZE / GRID_SIZE));
  return xx + GRID_SIZE * yy;
}

struct Food {
  Vector2D position;
  bool eaten = false;
};

struct Organism : Genome {
  Network brain;

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
  bool dead = false;

  ~Organism() { }

  Organism(DNA genes) : Genome(genes),
    brain({NUM_INPUTS, HIDDEN_NODES, NUM_OUTPUTS})
  {
    vector<double> weightGenes(
      genes.cbegin() + NUM_TRAITS,
      genes.cend()
    );

    brain.setWeights(weightGenes);

    fov = genes[TRAIT_FOV] * M_PI;
    sightLength = 300;

    for (int i = NUM_INPUTS; i--;) {
      input.push_back(0.0);
    }
  }

  float fitness() const override {
    float foodFitness = foodCollected / float(FOOD_AMOUNT);
    float fitness = powf(foodFitness, 2);
    return fitness;
  }

  void reset() override {
    angle = RANDOM_NUM * M_PI * 2;
    position.x = RANDOM_NUM * WORLD_SIZE;
    position.y = RANDOM_NUM * WORLD_SIZE;
    foodCollected = 0;
    clock = 0.f;
  }

  bool eat() {
    if (dead) { return false; }
    if (clock <= GENERATION_LIFESPAN) {
      foodCollected++;
      energy += 100.0;
    }
    return true;
  }

  void update() override {
    if (dead) { return; }
    brain.feedForward(input);
    brain.getResults(output);

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
    clock++;

    if (position.x < 0) { position.x = WORLD_SIZE; }
    if (position.y < 0) { position.y = WORLD_SIZE; }
    if (position.x > WORLD_SIZE) { position.x = 0; }
    if (position.y > WORLD_SIZE) { position.y = 0; }
    if (energy <= 0.0) {
      dead = true;
    }
  }

  void perceive(const vector<Food>& foods) {
    if (dead) { return; }
    for (int i = 0; i < FISH_NUM_EYES; i++) {
      float maxStrength = 0.f;
      float sensorDirection = angle + (-FISH_NUM_EYES / 2 + i) * (fov / float(FISH_NUM_EYES));
      for (auto& food : foods) {
        float x1 = position.x;
        float y1 = position.y;
        float cx = food.position.x;
        float cy = food.position.y;
        float diffx = x1 - cx;
        float diffy = y1 - cy;

        if (fabs(diffx) < sightLength && fabs(diffy) < sightLength) {
          float x2 = x1 + cosf(sensorDirection) * sightLength;
          float y2 = y1 + sinf(sensorDirection) * sightLength;
          if (lineCircleCollide(x1, y1, x2, y2, cx, cy, 16)) {
            float dist = sqrtf(diffx * diffx + diffy * diffy);
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

class NeatPond {
private:
  Population<Organism> population;
  vector<Food> foods;

public:
  NeatPond(): population(FISH_AMOUNT, DNA_LENGTH) {
    reset();
  }

  vector<Food>& getFood() {
    return foods;
  };

  vector<Organism>& getGenomes() {
    return population.genomes;
  };

  void spawnFood(Vector2D position) {
    Food food { position };
    foods.push_back(food);
  }

  void update() {
    for (auto& genome : population.genomes) {

      genome.perceive(foods);
      genome.update();

      for (auto& food : foods) {
        float mouthX = genome.position.x + cosf(genome.angle) * 8.f;
        float mouthY = genome.position.y + sinf(genome.angle) * 8.f;
        float distX = mouthX - food.position.x;
        float distY = mouthY - food.position.y;
        float distance = sqrt(distX * distX + distY * distY);
        if (distance <= 16 && bool(RANDOM_NUM > FOOD_EAT_DIFFICULTY)) {
          if (genome.eat()) {
            food.eaten = bool(RANDOM_NUM > FOOD_RESPAWN_RATE);
            food.position.x = RANDOM_NUM * WORLD_SIZE;
            food.position.y = RANDOM_NUM * WORLD_SIZE;
          }
        }
      }
    }

    foods.erase(
      remove_if(begin(foods), end(foods),
      [](Food& food) { return food.eaten; }),
      end(foods)
    );
  }

  float reset() {
    auto averageFitness = population.reproduce(population.genomes, MUTATION_RATE);

    foods.clear();

    for (int i = FOOD_AMOUNT; i--;) {
      spawnFood({
        float(RANDOM_NUM * WORLD_SIZE),
        float(RANDOM_NUM * WORLD_SIZE)
      });
    }

    for (auto& genome : population.genomes) {
      genome.reset();
    }

    return averageFitness;
  }
};

#endif
