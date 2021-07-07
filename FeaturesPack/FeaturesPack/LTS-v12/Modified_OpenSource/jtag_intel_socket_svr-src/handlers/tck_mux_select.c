/*
Copyright (c) 2018, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stddef.h>

#include "tck_mux_select.h"

STATUS tck_mux_select_initialize(const GPIO_State* state)
{
    STATUS status = ST_OK;
    if (state == NULL || state->gpio_fd < 0) {
        status = ST_ERR;
    }
    else if (state->gpios_config->tck_mux_select.is_present) {
        status = set_gpio_dir_output(state->gpio_fd, state->gpios_config->tck_mux_select.pin);
        if (status == ST_OK) {
            status = tck_mux_select_assert(state, false);
        }
    }
    return status;
}

STATUS tck_mux_select_deinitialize(const GPIO_State* state)
{
    STATUS status = ST_OK;
    if (state == NULL || state->gpio_fd < 0) {
        status = ST_ERR;
    }
    else if (state->gpios_config->tck_mux_select.is_present) {
        status = tck_mux_select_assert(state, false);
        if (status == ST_OK) {
            status = set_gpio_dir_input(state->gpio_fd, state->gpios_config->tck_mux_select.pin);
        }
    }
    return status;
}

STATUS tck_mux_select_assert(const GPIO_State* state, const bool assert)
{
    if (state == NULL || state->gpio_fd < 0)
        return ST_ERR;
    if (state->gpios_config->tck_mux_select.is_present) {
        return set_gpio_data(state->gpio_fd, state->gpios_config->tck_mux_select.pin,
                             assert == state->gpios_config->tck_mux_select.assert_high ? GPIO_HIGH : GPIO_LOW);
    }
    return ST_OK;
}
