/**
 * xlxRF24Server.cpp - Xlight RF2.4 server library based on via RF2.4 module
 * for communication with instances of xlxRF24Client
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2016 DTIT
 * Full contributor list:
 *
 * Documentation:
 * Support Forum:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * Dependancy
 * 1. Particle RF24 library, refer to
 *    https://github.com/stewarthou/Particle-RF24
 *    http://tmrh20.github.io/RF24/annotated.html
 *
 * DESCRIPTION
 * 1. Mysensors protocol compatible and extended
 * 2. Address algorithm
 * 3. PipePool (0 AdminPipe, Read & Write; 1-5 pipe pool, Read)
 * 4. Session manager, optional address shifting
 * 5.
 *
 * ToDo:
 * 1. Two pipes collaboration for security: divide a message into two parts
 * 2. Apply security layer, e.g. AES, encryption, etc.
 *
**/

#include "xlxRF24Server.h"
