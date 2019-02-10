#ifndef genetics_h
#define genetics_h

#include <vector>

#include "utils.hh"

using namespace std;

using DNA = vector<double>;

struct Genome {
  DNA genes;
  float fitnessScore = -1.f;
  Genome(DNA genes): genes(genes) {  }
  DNA crossOver(Genome& partner) const {
    DNA childGenes;
    int midpoint = rand() % genes.size();
    for (auto i = 0; i < genes.size(); i++) {
      childGenes.push_back(i > midpoint ? genes[i] : partner.genes[i]);
    }
    return childGenes;
  }
  virtual void reset() { };
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

template<class T>
struct Population {
  vector<T> genomes;
  Population(int populationSize, int dnaSize) {
    for (auto i = populationSize; i--;) {
      DNA genes;
      for (int j = dnaSize; j--;) {
        genes.push_back(RANDOM_NUM);
      }
      genomes.push_back(T(genes));
    }
    reset();
  }

  void reset() {
    for (auto& genome : genomes) { genome.reset(); }
  }

  float reproduce(vector<T>& genomes, float mutationRate) {
    auto numGenomes = genomes.size();
    auto fitnessSum = 0.0f;
    auto averageFitness = 0.0f;

    vector<T> matingPool;

    for (auto& g : genomes) {
      fitnessSum += g.calculateFitness();
    }
    averageFitness = fitnessSum / (float)numGenomes;

    sort(genomes.begin(), genomes.end(), sortByFitness);

    do {
      for (int i = 0; i < genomes.size(); i++) {
        if (RANDOM_NUM < (i + 1) / (float)numGenomes * 2) {
          matingPool.push_back(genomes[i]);
        }
      }
    } while (!matingPool.size());

    for (int i = 0; i < numGenomes; i++) {
      int a = rand() % matingPool.size();
      int b = rand() % matingPool.size();
      DNA genes = matingPool[a].crossOver(matingPool[b]);
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
