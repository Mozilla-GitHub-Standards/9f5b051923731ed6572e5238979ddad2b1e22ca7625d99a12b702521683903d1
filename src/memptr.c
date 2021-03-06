/*
 * Copyright (C) 2015-2016  Mozilla Foundation
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

#include <pdu/pdubuf.h>
#include "memptr.h"

static uintptr_t
_ceil_align(uintptr_t ptr, uintptr_t align)
{
  return ((ptr / align) + 1) * align;
}

void*
ceil_align(void* ptr, size_t align)
{
  return (void*)_ceil_align((uintptr_t)ptr, (uintptr_t)align);
}
