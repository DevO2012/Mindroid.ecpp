/*
 * Copyright (C) 2011 Daniel Himmelein
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

#include "mindroid/os/Clock.h"
#include <cmsis_os.h>

namespace mindroid {

uint64_t Clock::monotonicTime() {
	static uint32_t base = 0;
	static uint32_t offset = 0;
	static const uint32_t KERNEL_SYS_TICK_DIVIDER = osKernelSysTickMicroSec(1000);
	
	uint32_t now = osKernelSysTick() / KERNEL_SYS_TICK_DIVIDER;
	if (now == 0 && base == 0) {
		now = 1;
	}
	if (offset > now) {
		base++;
	}
	offset = now;
	return ((uint64_t) base << 32) | offset;
}

uint64_t Clock::realTime() {
	return monotonicTime();
}

} /* namespace mindroid */
