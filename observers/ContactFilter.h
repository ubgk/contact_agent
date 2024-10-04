// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Inria
#pragma once
#include <string>

#include "palimpsest/Dictionary.h"
#include "upkie/cpp/observers/Observer.h"

using palimpsest::Dictionary;
using upkie::cpp::observers::Observer;

/*! Estimate contact likelihood based on joint torque measurements.
 *
 */
class ContactFilter : public Observer {
 public:
  /*! Initialize observer.
   *
   * \param[in] p_contact Initial contact belief.
   */
  explicit ContactFilter(double p_contact = 0.5)
      : p_contact(p_contact), p_contact_smooth(p_contact) {}

  //! Prefix of outputs in the observation dictionary.
  inline std::string prefix() const noexcept final { return "contact_filter"; }

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

  // Contact belief
  double p_contact{};

  double p_contact_smooth{};
};
