/*
 *  Copyright © 2017-2024 Wellington Wallace
 *
 *  This file is part of Easy Effects.
 *
 *  Easy Effects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Easy Effects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Easy Effects. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <fftw3.h>
#include <sigc++/signal.h>
#include <sys/types.h>
#include <span>
#include <string>
#include <vector>
#include "pipe_manager.hpp"
#include "plugin_base.hpp"

class Spectrum : public PluginBase {
 public:
  Spectrum(const std::string& tag,
           const std::string& schema,
           const std::string& schema_path,
           PipeManager* pipe_manager,
           PipelineType pipe_type);
  Spectrum(const Spectrum&) = delete;
  auto operator=(const Spectrum&) -> Spectrum& = delete;
  Spectrum(const Spectrum&&) = delete;
  auto operator=(const Spectrum&&) -> Spectrum& = delete;
  ~Spectrum() override;

  void setup() override;

  void process(std::span<float>& left_in,
               std::span<float>& right_in,
               std::span<float>& left_out,
               std::span<float>& right_out) override;

  auto get_latency_seconds() -> float override;

  sigc::signal<void(uint, uint, std::vector<double>)> power;  // rate, nbands, magnitudes

 private:
  bool fftw_ready = false;

  fftwf_plan plan = nullptr;

  fftwf_complex* complex_output = nullptr;

  std::vector<float> in_mono;
  std::vector<float> real_input;
  std::vector<float> hann_window;
  std::vector<double> output;

  uint n_bands = 8192U;
};
