/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #pragma once

 #include <string>
 
 namespace minimal_systems {
 namespace init {
 
 enum FirstStageConsoleParam {
     DISABLED = 0,
     CONSOLE_ON_FAILURE = 1,
     IGNORE_FAILURE = 2,
     MAX_PARAM_VALUE = IGNORE_FAILURE,
 };
 
 void StartConsole(const std::string& cmdline);
 int FirstStageConsole(const std::string& cmdline, const std::string& bootconfig);
 
 }  // namespace init
 }  // namespace android