#ifndef genetics_h
#define genetics_h

#include <vector>

using namespace std;

using DNA = vector<double>;

struct Genome {
  DNA genes;
  float fitnessScore = -1.f;

  Genome(DNA genes): genes(genes) {  }

  virtual void reset() { };
  virtual void update() { };
  virtual float fitness() const = 0;

  inline float calculateFitness() {
    fitnessScore = fitness();
    return fitnessScore;
  }
};

DNA randomGenes(size_t size) {
  DNA genes;
  for (auto j = size; j--;) {
    genes.push_back(RANDOM_NUM);
  }
  return genes;
}

DNA crossOver(DNA& genesA, DNA& genesB) {
  DNA offspringGenes;
  int midpoint = rand() % genesA.size();
  for (auto i = 0; i < genesA.size(); i++) {
    offspringGenes.push_back(i > midpoint ? genesA[i] : genesB[i]);
  }
  return offspringGenes;
}

DNA mutate(DNA genes, float mutationRate) {
  DNA mutatedGenes;
  for (int j = 0; j < genes.size(); j++) {
    if (mutationRate > RANDOM_NUM) {
      mutatedGenes.push_back(RANDOM_NUM);
    } else {
      mutatedGenes.push_back(genes[j]);
    }
  }
  return mutatedGenes;
}

bool sortByFitness(const Genome &genomeA, const Genome &genomeB) {
  return genomeA.fitnessScore < genomeB.fitnessScore;
}

template<class T>
struct Population {
  vector<T> genomes;

  Population(size_t populationSize, size_t dnaSize) {
    for (auto i = populationSize; i--;) {
      genomes.push_back(T(randomGenes(dnaSize)));
    }
    reset();
  }

  void reset() {
    for (auto& genome : genomes) { genome.reset(); }
  }

  float reproduce(vector<T>& genomes, float mutationRate) {
    auto numGenomes = genomes.size();
    auto fitnessSum = 0.0f;
    vector<T> matingPool;

    for (auto& g : genomes) {
      fitnessSum += g.calculateFitness();
    }

    sort(genomes.begin(), genomes.end(), sortByFitness);

    while (matingPool.size() == 0) {
      for (int i = 0; i < genomes.size(); i++) {
        if (RANDOM_NUM < (i + 1) / (float)numGenomes * 2) {
          matingPool.push_back(genomes[i]);
        }
      }
    }

    genomes.clear();

    for (int i = 0; i < numGenomes; i++) {
      int a = rand() % matingPool.size();
      int b = rand() % matingPool.size();
      auto genes = mutate(
        crossOver(matingPool[a].genes, matingPool[b].genes),
        mutationRate
      );
      genomes.push_back(T(genes));
    }

    return fitnessSum / (float)numGenomes;
  }
};

#endif
