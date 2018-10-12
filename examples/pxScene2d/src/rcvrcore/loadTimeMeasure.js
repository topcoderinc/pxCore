/*

pxCore Copyright 2005-2018 John Robinson

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

// The global object to store the load time measurement
"use strict";

module.exports = {
  // [Performance Tests]
  // A map to store which components need a load time measurement
  // Map key:
  //   - component type, such as 'image', 'image9', 'scene' etc.
  // Map value:
  //   - true: means the component needs a load time measurement.
  //   - false/undefined/null: means the component doesn't need a load time measurement.
  loadTimeMeasurementFlagMap: {},

  // [Performance Tests]
  // A map to store the count of components creation
  // The creation only counts when the component type is in loadTimeMeasurementFlagMap
  // Map key:
  //   - component type, such as 'image', 'image9', 'scene' etc.
  // Map value:
  //   - the total count of creation; number
  componentCreationCountMap: {},

  // [Performance Tests]
  // A map to store the accumulated load time of components with a specific type
  // It only counts when the component type is in loadTimeMeasurementFlagMap
  // Map key:
  //   - component type, such as 'image', 'image9', 'scene' etc.
  // Map value:
  //   - the accumulated load time of creation; number
  componentloadTimeMap: {},

  /**
   * [Performance Tests]
   * Enable the load time measurement for a component
   * @param {*} componentType The compoenent type, such as 'image', 'image9', 'scene' etc.
   */
  enableLoadTimeMeasurement: function (componentType) {
    // Set the flag.
    this.loadTimeMeasurementFlagMap[componentType] = true;
  },

  /**
   * [Performance Tests]
   * Disable the load time measurement for a component
   * @param {*} componentType The compoenent type, such as 'image', 'image9', 'scene' etc.
   */
  disableLoadTimeMeasurement: function (componentType) {
    // Set the flag.
    this.loadTimeMeasurementFlagMap[componentType] = false;
  },

  /**
   * [Performance Tests]
   * Reset the load time counter, including the # of created components
   * and the accumulated load time of components
   */
  resetCounter: function () {
    this.componentCreationCountMap = {};
    this.componentloadTimeMap = {};
  },
};
