/* libcgate - library to send and receve messages from Clipsal's C-Gate server.
 * Copyright (C) 2017, Andrew Tarabaras.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _LIBCGATE_H
#define _LIBCGATE_H

typedef enum
  {
    cbus_measurement_ce_centigrade,
    cbus_measurement_ce_amp,
    cbus_measurement_ce_degree,
    cbus_measurement_ce_coulomb,
    cbus_measurement_ce_true_false,
    cbus_measurement_ce_farad,
    cbus_measurement_ce_henry,
    cbus_measurement_ce_hertz,
    cbus_measurement_ce_joule,
    cbus_measurement_ce_katal,
    cbus_measurement_ce_kg_m3,
    cbus_measurement_ce_kg,
    cbus_measurement_ce_litre,
    cbus_measurement_ce_litre_h,
    cbus_measurement_ce_litre_m,
    cbus_measurement_ce_litre_s,
    cbus_measurement_ce_lux,
    cbus_measurement_ce_metre,
    cbus_measurement_ce_metre_m,
    cbus_measurement_ce_metre_s,
    cbus_measurement_ce_metre_s2,
    cbus_measurement_ce_mole,
    cbus_measurement_ce_newton_metre,
    cbus_measurement_ce_newton,
    cbus_measurement_ce_ohm,
    cbus_measurement_ce_pascal,
    cbus_measurement_ce_percent,
    cbus_measurement_ce_decibel,
    cbus_measurement_ce_ppm,
    cbus_measurement_ce_rpm,
    cbus_measurement_ce_second,
    cbus_measurement_ce_minute,
    cbus_measurement_ce_hour,
    cbus_measurement_ce_sievert,
    cbus_measurement_ce_steradian,
    cbus_measurement_ce_tesla,
    cbus_measurement_ce_volt,
    cbus_measurement_ce_watt_hour,
    cbus_measurement_ce_watt,
    cbus_measurement_ce_weber,
    cbus_measurement_ce_no_unit=0xFE,
    cbus_measurement_ce_custom
  } cbus_measurement_unit_type;

//
// Connect to Cgate server. Takes IP, Port, project string and network number.
// Returns a socket to be used in all other commands

int32_t cgate_connect(char* ip, int portno, int8_t *project, uint8_t net);

void cgate_lighting_register_event_handler(void (*f)(uint8_t,
                                                     uint8_t,
                                                     uint8_t,
                                                     uint8_t,
                                                     uint8_t));

void cgate_measurement_register_handler(void (*f)(uint8_t,
                                                  uint8_t,
                                                  uint8_t,
                                                  uint8_t,
                                                  int32_t,
                                                  int8_t,
                                                  cbus_measurement_unit_type));


int32_t cgate_set_group(int sockfd, uint8_t net, uint8_t app, uint8_t group, uint8_t value);
int32_t cgate_set_ramp(int sockfd, uint8_t net, uint8_t app, uint8_t group, uint8_t value, uint8_t ramprate);
int32_t cgate_send_measurement(int sockfd, uint8_t net, uint8_t app, uint8_t device, uint8_t channel, int32_t value, int8_t exponent, cbus_measurement_unit_type);
#endif
