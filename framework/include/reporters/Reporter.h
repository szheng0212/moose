//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "OutputInterface.h"
#include "ReporterData.h"
#include "InputParameters.h"
class FEProblemBase;

/**
 * Reporter objects allow for the declaration of arbitrary data types that are aggregate values
 * for a simulation. These aggregate values are then available to other objects for use. They
 * operate with the typical producer/consumer relationship. The Reporter object produces values
 * that other objects consume.
 *
 * Originally, MOOSE included a Postprocessor system that allowed for an object to produce a
 * single scalar value for consumption by other objects. Then a companion system was created,
 * the VectorPostprocessor system, that allowed for an object to produced many std::vector<Real>
 * values. The Reporter system is the generalization of these two ideas and follows closely the
 * original design of the VectorPostprocessor system.
 *
 * In practice, the Reporter system provided the following features over the two previous systems.
 *
 * 1. It can create arbitrary data types rather than only Real and std::vector<Real>
 * 2. It can create many values per object. This was possible for VectorPostprocessor objects but
 *    not for Postprocessors. Therefore, this allows a single Reporter to provide multiple values
 *    and consolidate similar items. For example, a "counter" object replaced several individual
 *    Postprocessor objects.
 * 3. The ReporterContext system allows for each data value to have special operation within a
 *    single object. Previously the VectorPostprocessor system had capability to perform
 *    broadcasting, but it was applied to all vectors in the object.
 */
class Reporter : public OutputInterface
{
public:
  static InputParameters validParams();
  Reporter(const InputParameters & parameters);
  virtual ~Reporter() = default;

protected:
  ///@{
  /**
   * Method to define a value that the Reporter object is to produced.
   * @tparam T The C++ type of the value to be produced
   * @tparam S (optional) Context type for performing special operations. For example
   *           using the ReporterBroadcastContext will automatically broadcast the produced value
   *           from the root processor.
   * @param name A unique name for the value to be produced.
   * @param mode (optional) The mode that indicates how the value produced is represented in
   *             parallel, there is more information about this below
   * @param args (optional) Any number of optional arguments passed into the ReporterContext given
   *             by the S template parameter. If S = ReporterContext then the first argument
   *             can be used as the default value (see ReporterContext.h).
   *
   * The 'mode' indicates how the value that is produced is represented in parallel. It is the
   * reponsibility of the Reporter object to get it to that state. The ReporterContext objects
   * are designed to help with this. The mode can be one of the following:
   *
   *     ReporterMode::ROOT Indicates that the value produced is complete/correct on the
   *                        root processor for the object.
   *     ReporterMode::REPLICATED Indicates that the value produced is complete/correct on
   *                              all processors AND that the value is the same on all
   *                              processors
   *     ReporterMode::DISTRIBUTED Indicates that the value produced is complete/correct on
   *                               all processors AND that the value is NOT the same on all
   *                               processors
   *
   * WARNING! Using the "default value" in ReporterContext:
   * The Reporter system, like the systems before it, allow for objects that consume values to be
   * constructed prior to the produce objects. When a value is requested either by a producer
   * (Reporter) or consumer (ReporterInterface) the data is allocated. As such the assigned default
   * value from the producer should not be relied upon on the consumer side during object
   * construction.
   *
   * NOTE:
   * The ReporterContext is just a mechanism to allow for handling of values in special ways. In
   * practice it might be better to have specific methods for these special cases. For example,
   * a declareBroadcastValue, etc. Please refer to the ReporterData object for more information
   * on how the data system operates for Reporter values.
   */
  template <typename T, template <typename> class S = ReporterContext, typename... Args>
  T & declareValue(const std::string & param_name, Args &&... args0);
  template <typename T, template <typename> class S = ReporterContext, typename... Args>
  T & declareValue(const std::string & param_name, ReporterMode mode, Args &&... args0);
  template <typename T, template <typename> class S = ReporterContext, typename... Args>
  T & declareValueByName(const ReporterValueName & value_name, Args &&... args0);
  template <typename T, template <typename> class S = ReporterContext, typename... Args>
  T & declareValueByName(const ReporterValueName & value_name, ReporterMode mode, Args &&... args0);
  ///@}

private:
  /// Ref. to MooseObject params
  const InputParameters & _reporter_params;

  /// The name of the MooseObject, from "_object_name" param
  const std::string & _reporter_name;

  /// Needed for access to FEProblemBase::getReporterData
  FEProblemBase & _reporter_fe_problem;

  /// Data storage
  ReporterData & _reporter_data;
};

template <typename T, template <typename> class S, typename... Args>
T &
Reporter::declareValue(const std::string & param_name, Args &&... args)
{
  const ReporterValueName & value_name = _reporter_params.get<ReporterValueName>(param_name);
  return declareValueByName<T, S>(value_name, REPORTER_MODE_UNSET, args...);
}

template <typename T, template <typename> class S, typename... Args>
T &
Reporter::declareValue(const std::string & param_name, ReporterMode mode, Args &&... args)
{
  const ReporterValueName & value_name = _reporter_params.get<ReporterValueName>(param_name);
  return declareValueByName<T, S>(value_name, mode, args...);
}

template <typename T, template <typename> class S, typename... Args>
T &
Reporter::declareValueByName(const ReporterValueName & value_name, Args &&... args)
{
  return declareValueByName<T, S>(value_name, REPORTER_MODE_UNSET, args...);
}

template <typename T, template <typename> class S, typename... Args>
T &
Reporter::declareValueByName(const ReporterValueName & value_name,
                             ReporterMode mode,
                             Args &&... args)
{
  ReporterName state_name(_reporter_name, value_name);
  buildOutputHideVariableList({state_name.getCombinedName()});
  return _reporter_data.declareReporterValue<T, S>(state_name, mode, args...);
}
