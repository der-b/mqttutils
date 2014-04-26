/* Copyright 2014 Bernd Lehmann (der-b@der-b.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __config_h__
#define __config_h__

#include <sys/time.h>

/* redability stuff */
#define ON  1
#define OFF 0

#define LOG_FD stdout

/* configure loglevel */
#ifdef TEST
  #define LOGLEVEL_INFO  OFF
  #define LOGLEVEL_DEBUG OFF
  #define LOGLEVEL_WARN  OFF
  #define LOGLEVEL_ERROR OFF
  #define LOGLEVEL_CRIT  OFF
#endif /* TEST */

#ifndef MOSQ_MQTT_ID_MAX_LENGTH
#define MOSQ_MQTT_ID_MAX_LENGTH 23
#endif

#endif
