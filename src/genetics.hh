#ifndef genetics_h
#define genetics_h

#include <iostream>
#include <vector>

using namespace std;

using DNA = vector<double>;

struct Genome {
  DNA genes;
  float _id;
  float fitnessScore = -1.f;
  Genome(DNA genes): genes(genes), _id(RANDOM_NUM) {  }
  DNA crossOver(Genome& partner) const {
    DNA childGenes;
    int midpoint = rand() % genes.size();
    for (auto i = 0; i < genes.size(); i++)
      childGenes.push_back(i > midpoint ? genes[i] : partner.genes[i]);
    return childGenes;
  }
  virtual void begin() { };
  virtual void update() { };
  virtual float fitness() const = 0;
  float calculateFitness() {
    fitnessScore = fitness();
    return fitnessScore;
  }
};

bool sortByFitness(const Genome &genomeA, const Genome &genomeB) {
  return genomeA.fitnessScore < genomeB.fitnessScore;
}

template<class T, int POPULATION_SIZE, int DNA_LENGTH>
struct Population {
  vector<T> genomes;
  Population() {
    for (auto i = POPULATION_SIZE; i--;) {
      DNA genes;
      for (int j = DNA_LENGTH; j--;) {
        genes.push_back(RANDOM_NUM);
      }
      genomes.push_back(T(genes));
    }
    begin();
  }

  void begin() {
    for (auto& genome : genomes) { genome.begin(); }
  }
};

struct Evolution {
  template<class T>
  float reproduce(vector<T>& genomes, float mutationRate) {
    auto numGenomes = genomes.size();
    auto fitnessSum = .0f;
    auto averageFitness = .0f;
    auto averageRed = .0f;
    auto averageGreen = .0f;
    auto averageBlue = .0f;

    vector<T> matingPool;

    // calculate fitnesses
    for (auto& g : genomes) {
      fitnessSum += g.calculateFitness();
    }
    averageFitness = fitnessSum / (float)numGenomes;

    // sort by fitness
    sort(genomes.begin(), genomes.end(), sortByFitness);

    do {
      for (int i = 0; i < genomes.size(); i++) {
        // add surviver to mating pool
        // float rank = genomes[i].getFitness() / averageFitness;
        if (RANDOM_NUM < (i + 1) / (float)numGenomes * 2) {
          matingPool.push_back(genomes[i]);
        }
      }
    } while (!matingPool.size());

    // crossover
    for (int i = 0; i < numGenomes; i++) {
      int a = rand() % matingPool.size();
      int b = rand() % matingPool.size();
      DNA genes = matingPool[a].crossOver(matingPool[b]);
      // mutatation
      for (int j = 0; j < genes.size(); j++) {
        if (mutationRate > RANDOM_NUM) {
          genes[j] = RANDOM_NUM;
        }
      }
      genomes[i] = T(genes);
    }

    return averageFitness;
  }
};

#endif
