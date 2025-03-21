/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_TAG_MAP_FILE "/etc/event.d/event-log-tags"

struct EventTagMap;
typedef struct EventTagMap EventTagMap;

/*
 * Open the specified file as an event log tag map.
 *
 * Returns NULL on failure.
 */
EventTagMap* linux_openEventTagMap(const char* fileName);

/*
 * Close the map.
 */
void linux_closeEventTagMap(EventTagMap* map);

/*
 * Look up a tag by index.  Returns the tag string & string length, or NULL if
 * not found.  Returned string is not guaranteed to be nul terminated.
 */
const char* linux_lookupEventTag_len(const EventTagMap* map, size_t* len, unsigned int tag);

/*
 * Look up a format by index. Returns the format string & string length,
 * or NULL if not found. Returned string is not guaranteed to be nul terminated.
 */
const char* linux_lookupEventFormat_len(const EventTagMap* map, size_t* len, unsigned int tag);

#ifdef __cplusplus
}
#endif