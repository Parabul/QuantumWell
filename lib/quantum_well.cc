#include "quantum_well.h"

#include <cassert>
#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <numbers>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
#endif

CircularQuantumWell::CircularQuantumWell(int grid_size, double r)
    : gridSize(grid_size), radius(r) {
  frameBuffer.resize(gridSize * gridSize * 4, 255);
}

// Rigorous numerical root finding for J_l(x) = 0
double CircularQuantumWell::getBesselRoot(int l, int n) {
  int abs_l = std::abs(l);

  // 1. McMahon's approximation for an excellent initial guess
  double beta = (n + 0.5 * abs_l - 0.25) * std::numbers::pi;
  double x = beta - (4.0 * abs_l * abs_l - 1.0) / (8.0 * beta);

  // 2. Newton-Raphson Iteration
  for (int i = 0; i < 20; ++i) {
    double j_l = ::jn(abs_l, x);
    // Derivative using recurrence relation: J'_l = 0.5 * (J_{l-1} - J_{l+1})
    double j_l_prime = 0.5 * (::jn(abs_l - 1, x) - ::jn(abs_l + 1, x));

    double dx = j_l / j_l_prime;
    x -= dx;

    // Break early if we've reached machine precision
    if (std::abs(dx) < 1e-8) break;
  }
  return x;
}

void CircularQuantumWell::addState(int l, int n, double real_amp,
                                   double imag_amp) {
  Eigenstate state;
  state.l = l;
  state.n = n;
  state.coefficient = Complex(real_amp, imag_amp);

  // Energy quantization
  double root = getBesselRoot(l, n);
  state.energy = (root * root) / (radius * radius);

  // Exact Analytical Normalization
  double j_l_plus_1 = ::jn(std::abs(l) + 1, root);
  double normalization =
      1.0 / (radius * std::sqrt(std::numbers::pi) * std::abs(j_l_plus_1));

  state.spatialGrid.resize(gridSize, vector<Complex>(gridSize, 0.0));
  double center = gridSize / 2.0;

  for (int x = 0; x < gridSize; ++x) {
    for (int y = 0; y < gridSize; ++y) {
      double dx = (x - center) / center * radius;
      double dy = (y - center) / center * radius;
      double r = std::sqrt(dx * dx + dy * dy);
      double theta = std::atan2(dy, dx);

      if (r <= radius) {
        // Evaluate the full normalized eigenstate
        double radial_part = ::jn(l, root * r / radius);
        Complex angular_part = std::exp(I * (double)l * theta);

        state.spatialGrid[x][y] = normalization * radial_part * angular_part;
      }
    }
  }
  activeStates.push_back(state);
}

void CircularQuantumWell::updateTime(double dt) {
  for (auto& state : activeStates) {
    Complex phaseRotation = std::exp(-I * state.energy * dt);
    state.coefficient *= phaseRotation;
  }
}

Complex CircularQuantumWell::getWavefunctionAt(int x, int y) {
  Complex psi(0.0, 0.0);
  for (const auto& state : activeStates) {
    psi += state.coefficient * state.spatialGrid[x][y];
  }
  return psi;
}

void CircularQuantumWell::renderFrame(double dt) {
  updateTime(dt);
  double center = gridSize / 2.0;

  // Calculate our dynamic multiplier based on the PREVIOUS frame's peak.
  // We use 255.0 to map the highest peak exactly to pure white/full brightness.
  double brightnessScale = 255.0 / peakMagnitude;

  // Variable to find the highest peak in the CURRENT frame
  double currentFramePeak = 0.0001;  // Small baseline to prevent divide-by-zero

  for (int y = 0; y < gridSize; ++y) {
    for (int x = 0; x < gridSize; ++x) {
      int index = (y * gridSize + x) * 4;

      double dx = x - center;
      double dy = y - center;
      if (dx * dx + dy * dy > center * center) {
        frameBuffer[index + 0] = 0;
        frameBuffer[index + 1] = 0;
        frameBuffer[index + 2] = 0;
        frameBuffer[index + 3] = 255;
        continue;
      }

      Complex psi = getWavefunctionAt(x, y);

      double mag = std::abs(psi);
      double phase = std::arg(psi);

      // Track the highest magnitude we see in this frame
      if (mag > currentFramePeak) {
        currentFramePeak = mag;
      }

      // Apply the dynamic scale factor
      double intensity = std::min(mag * brightnessScale, 255.0);

      // 1. Convert phase (-pi to pi) to a Hue degree (0 to 360)
      double hue = (phase + std::numbers::pi) * (180.0 / std::numbers::pi);

      // 2. Map intensity to a normalized Value (0.0 to 1.0)
      double v = intensity / 255.0;

      // 3. HSV to RGB Math (Saturation is assumed to be 1.0 for max vibrancy)
      double c = v; // Chroma
      double x_val = c * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));

      double r1 = 0, g1 = 0, b1 = 0;

      if      (hue >= 0   && hue < 60)  { r1 = c; g1 = x_val; b1 = 0; }
      else if (hue >= 60  && hue < 120) { r1 = x_val; g1 = c; b1 = 0; }
      else if (hue >= 120 && hue < 180) { r1 = 0; g1 = c; b1 = x_val; }
      else if (hue >= 180 && hue < 240) { r1 = 0; g1 = x_val; b1 = c; }
      else if (hue >= 240 && hue < 300) { r1 = x_val; g1 = 0; b1 = c; }
      else if (hue >= 300 && hue <= 360){ r1 = c; g1 = 0; b1 = x_val; }

      frameBuffer[index + 0] = static_cast<uint8_t>(r1 * 255.0);
      frameBuffer[index + 1] = static_cast<uint8_t>(g1 * 255.0);
      frameBuffer[index + 2] = static_cast<uint8_t>(b1 * 255.0);
      frameBuffer[index + 3] = 255;
    }
  }

  // Smoothly interpolate the peak magnitude for the next frame.
  // Blending 90% of the old peak with 10% of the new peak prevents aggressive
  // flickering when the wavepacket sharply collides with the walls.
  peakMagnitude = (peakMagnitude * 0.90) + (currentFramePeak * 0.10);
}

size_t CircularQuantumWell::getBufferPointer() const {
  return reinterpret_cast<size_t>(frameBuffer.data());
}

void CircularQuantumWell::createWavepacket(double click_x, double click_y,
                                           double sigma) {
  // click_x and click_y should be in physical coordinates [-radius, radius]
  // sigma determines the width of the initial particle distribution

  double center = gridSize / 2.0;

  // Calculate the physical area of a single pixel for our integral (dA = dx *
  // dy)
  double pixel_width = (2.0 * radius) / gridSize;
  double dA = pixel_width * pixel_width;

  // Project the Gaussian onto every active state in our basis
  for (auto& state : activeStates) {
    Complex overlap(0.0, 0.0);

    for (int x = 0; x < gridSize; ++x) {
      for (int y = 0; y < gridSize; ++y) {
        // Convert pixel coordinates (x,y) to physical coordinates (px, py)
        double px = (x - center) / center * radius;
        double py = (y - center) / center * radius;

        // Only integrate inside the bounds of the circular well
        if (px * px + py * py <= radius * radius) {
          // 1. Calculate the value of the Gaussian at this pixel
          double dist_sq =
              (px - click_x) * (px - click_x) + (py - click_y) * (py - click_y);
          double gaussian_val = std::exp(-dist_sq / (2.0 * sigma * sigma));
          Complex psi_click(gaussian_val, 0.0);

          // 2. Add to the integral: psi_ln* * psi_click * dA
          overlap += std::conj(state.spatialGrid[x][y]) * psi_click * dA;
        }
      }
    }

    // The result of the integral is the new starting amplitude for this state!
    state.coefficient = overlap;
  }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(my_module) {
  // Bind the class and its public methods
  class_<CircularQuantumWell>("CircularQuantumWell")
      .constructor<int, double>()
      .function("addState", &CircularQuantumWell::addState)
      .function("renderFrame", &CircularQuantumWell::renderFrame)
      .function("createWavepacket", &CircularQuantumWell::createWavepacket)
      .function("getBufferPointer", &CircularQuantumWell::getBufferPointer);
}
#endif