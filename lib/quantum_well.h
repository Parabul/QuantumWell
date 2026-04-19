#ifndef QUANTUM_WELL_H
#define QUANTUM_WELL_H

#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

using namespace std;

// Define complex numbers for ease of use
typedef complex<double> Complex;
const complex<double> I(0.0, 1.0);

// Represents a single Eigenstate |l, n>
struct Eigenstate {
  int l;                // Angular quantum number
  int n;                // Radial quantum number
  double energy;        // Energy of the state (determines rotation speed)
  Complex coefficient;  // Current complex amplitude (cln)

  // Precomputed spatial grid for this specific state to save frame-time
  vector<vector<Complex>> spatialGrid;
};

class CircularQuantumWell {
 private:
  int gridSize;
  double radius;
  vector<Eigenstate> activeStates;

  // Flat RGBA pixel array representing the canvas frame
  vector<uint8_t> frameBuffer;

  double peakMagnitude = 1.0;

 public:
  CircularQuantumWell(int grid_size, double r);

  void createWavepacket(double click_x, double click_y, double sigma);

  Complex getWavefunctionAt(int x, int y);

  // Internal physics methods
  void updateTime(double dt);

  // Mock function to get the n-th root of the l-th Bessel function
  double getBesselRoot(int l, int n);

  // Passing primitive doubles is easier for Emscripten than passing Complex
  // objects
  void addState(int l, int n, double real_amp, double imag_amp);

  // Generates the frame and populates frameBuffer
  void renderFrame(double dt);

  // Returns the memory address of the frame buffer for JavaScript
  size_t getBufferPointer() const;
};

#endif