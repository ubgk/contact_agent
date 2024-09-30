// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria

#pragma once

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "observers/NpzInterpolator.h"
#include "palimpsest/Dictionary.h"
#include "upkie/cpp/observers/Observer.h"

using palimpsest::Dictionary;
using upkie::cpp::observers::Observer;

/*! Estimate contact likelihood based on joint torque measurements.
 *
 */
class MeasurementModel : public Observer {
public:
  //! Parameters to the measurement model
  struct Parameters {
    //! Path to the executable
    std::string argv0;

    //! Time step between observations
    double dt = 0.001;

    //! List of joint names, in order of axes, should be the same length as
    //! axis_keys and cutoff_periods
    std::vector<std::string> joint_names = {"wheel", "knee"};

    //! Name of the leg to consider
    std::string leg_name = "left";

    //! Axis keys to be loaded from the .npz file, should be the same length as
    //! joint_names and cutoff_periods.
    std::vector<std::string> axis_keys = {"wheel_torque", "knee_torque"};

    /*! Value keys to be loaded from the .npz file, the first value is the
     * contact likelihood, the second is the no contact likelihood array. Other
     * values can be added to the list to be queried.
     */
    std::vector<std::string> value_keys = {
        "contact_likelihood", "no_contact_likelihood", "P_contact"};

    //! Path to the measurement model
    std::string model_path =
        "contact_agent/observers/data/simulation_measurement_model.npz";

    //! Torque filter cutoff periods, should be the same length as joint_names
    //! and axis_keys
    std::vector<double> cutoff_periods = {0.025, 0.025};
  };

  struct Likelihoods {
    double contact;
    double no_contact;
  };

  /*! Initialize observer.
   *
   * \param[in] params Observer parameters.
   */
  explicit MeasurementModel(const Parameters &params);

  // Destructor
  // ~MeasurementModel() override = default;

  //! Prefix of outputs in the observation dictionary.
  inline std::string prefix() const noexcept final {
    return "measurement_model";
  }

  /*! Read inputs from other observations.
   *
   * \param[in] observation Dictionary to read other observations from.
   */
  void read(const Dictionary &observation) override;

  /*! Write outputs, called if reading was successful.
   *
   * \param[out] observation Dictionary to write observations to.
   */
  void write(Dictionary &observation) override;

  /*! Query the likelihoods for a given point.
   *
   * \param[in] point Point to query, should have the same length and order as
   * Parameters::axis_keys.
   *
   * \return Pair of likelihoods: contact, no contact.

  */
  Likelihoods query_likelihoods(const std::vector<double> &point) const;

private:
  //! Observer parameters
  // Parameters params_;

  //! Name of the leg to consider
  std::string leg_name;

  //! List of joint names, in order of axes
  std::vector<std::string> joint_names;

  //! Cutoff periods for the low-pass filter
  std::vector<double> cutoff_periods;

  //! Filtered torques
  std::vector<double> filtered_torques;

  //! NpzInterpolator
  std::unique_ptr<NpzInterpolator> interpolator;

  //! Likelihoods
  Likelihoods likelihoods;

  //! Time step
  double dt;
};
