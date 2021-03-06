//--------------------------------------------------------------------------
// Copyright (C) 2014-2015 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2004-2013 Sourcefire, Inc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
//

/*
 * ssh_config.h: Definitions of SSH conf
 * Author: Chris Sherwin
 */

#ifndef SSH_CONFIG_H
#define SSH_CONFIG_H

/*
 * Global SSH preprocessor configuration.
 *
 * MaxEncryptedPackets: Maximum number of encrypted packets examined per
 *				session.
 * MaxClientBytes:	Maximum bytes of encrypted data that can be
 *				sent by client without a server response.
 * MaxServerVersionLen: Maximum length of a server's version string.
 *              Configurable threshold for Secure CRT-style overflow.
 */
struct SSH_PROTO_CONF
{
    uint16_t MaxEncryptedPackets;
    uint16_t MaxClientBytes;
    uint16_t MaxServerVersionLen;
};

#define SSH_DEFAULT_MAX_ENC_PKTS    25
#define SSH_DEFAULT_MAX_CLIENT_BYTES    19600
#define SSH_DEFAULT_MAX_SERVER_VERSION_LEN 80


#endif

