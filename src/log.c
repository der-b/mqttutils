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
#include "log.h"
#include <stdint.h>

void log_buf(void *buf, size_t length) {
  int i;
  uint8_t *buff = (uint8_t *)buf;
  
  for( i = 0; i < length; i++ ) {
    fprintf(LOG_FD, "%02x ", buff[i]);
  }
}

