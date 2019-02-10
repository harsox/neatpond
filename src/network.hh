#ifndef network_h
#define network_h

#include <cmath>
#include <iostream>
#include <vector>

#include "utils.hh"

using namespace std;

const float WEIGHT_RANGE = 20.0;

double sigmoid(double x) {
  return 1.f / (1.f + exp(-x));
}

class Neuron;
using Layer = vector<Neuron>;

class Neuron {
private:
  unsigned index;
  double output;
  vector<double> connectionWeights;

public:
  ~Neuron() { }

  Neuron(unsigned index, unsigned numOutputs) : index (index) {
    for (auto i(0); i < numOutputs; i++) {
      // create a new connection with a random weight
      connectionWeights.push_back(rand() / double(RAND_MAX));
    }
  }
  void feedForward(Layer &previousLayer) {
    auto sum = 0.f;
    // iter through the neurons in the previous layer
    // and add their output times connection weight to sum
    for (auto n = 0; n < previousLayer.size(); ++n) {
      sum += previousLayer[n].output * previousLayer[n].connectionWeights[index];
    }
    output = sigmoid(sum);
  }

  void setConnectionWeights(vector<double> &weights) {
    assert(weights.size() >= connectionWeights.size());
    for (auto c = 0; c < connectionWeights.size(); ++c) {
      connectionWeights[c] = (-1 + weights.back() * 2) * WEIGHT_RANGE;
      weights.pop_back();
    }
  }
  void setOutput(double i_output) { output = i_output; }
  double getOutput() const { return output; }
  vector<double> getConnectionWeights() const { return connectionWeights; }
};

class Network {
private:
  vector<Layer> layers;
public:
  ~Network() { }

  Network(vector<unsigned> topology) {
    auto numLayers = topology.size();
    for (int l = 0; l < numLayers; l++) {
      layers.push_back(Layer());
      // number of outputs equals neurons in the next layer
      auto numOutputs = l == numLayers - 1 ? 0 : topology[l + 1];
      // number of neurons plus one biased neuron
      unsigned numNeurons = topology[l] + 1;
      for (int n = 0; n < numNeurons; n++) {
        layers.back().push_back(Neuron(n, numOutputs));
      }
      // biased output
      layers.back().back().setOutput(1.f);
    }
  }

  void feedForward(vector<double> &input) {
    Layer &inputLayer = layers[0];
    // make sure that the input has the same number of
    // values as our input layer has neurons (minus the biased neuron)
    if (input.size() != inputLayer.size() - 1) {
      cerr << "Input doesnt match topology" << endl;
      assert(0 != 0);
    }
    // set input layer's neutrons output to input
    for (int i = 0; i < input.size(); i++) {
      inputLayer[i].setOutput(input[i]);
    }
    // feed forward
    for (int l = 1u; l < layers.size(); l++) {
      Layer &previousLayer = layers[l - 1];
      // we skip the biased neuron
      for (int n = 0; n < layers[l].size() - 1; n++) {
        layers[l][n].feedForward(previousLayer);
      }
    }
  }

  void setWeights(vector<double> weights) {
    for (int l = 0; l < layers.size() - 1; l++) {
      for (int n = 0; n < layers[l].size(); n++) {
        layers[l][n].setConnectionWeights(weights);
      }
    }
  }

  void getResults(vector<double> &results) const {
    results.clear();
    for (int n = 0; n < layers.back().size() - 1; ++n) {
      results.push_back(layers.back()[n].getOutput());
    }
  }

  const vector<Layer>& getLayers() const {
    return layers;
  };
};

#endif
