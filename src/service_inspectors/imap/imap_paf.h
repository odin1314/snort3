/****************************************************************************
 * Copyright (C) 2015 Cisco and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 ****************************************************************************/

#ifndef IMAP_PAF_H
#define IMAP_PAF_H

#include "snort_types.h"
#include "stream/stream_api.h"
#include "stream/stream_splitter.h"
#include "file_api/file_api.h"

struct ImapDataInfo
{
    int paren_cnt;            /* The open parentheses count in fetch */
    const char* next_letter;        /* The current command in fetch */
    bool found_len;
    uint32_t length;
    bool esc_nxt_char;        /* true if the next charachter has been escaped */
};

/* State tracker for SMTP PAF */
typedef enum _ImapPafState
{
    IMAP_PAF_REG_STATE,           /* default state. eat until LF */
    IMAP_PAF_DATA_HEAD_STATE,     /* parses the fetch header */
    IMAP_PAF_DATA_LEN_STATE,      /* parse the literal length */
    IMAP_PAF_DATA_STATE,          /* search for and flush on MIME boundaries */
    IMAP_PAF_FLUSH_STATE,         /* flush if a termination sequence is found */
    IMAP_PAF_CMD_IDENTIFIER,      /* determine the line identifier ('+', '*', tag) */
    IMAP_PAF_CMD_TAG,             /* currently analyzing tag . identifier*/
    IMAP_PAF_CMD_STATUS,          /* currently parsing second argument */
    IMAP_PAF_CMD_SEARCH           /* currently searching data for a command */
} ImapPafState;

typedef enum _ImapDataEnd
{
    IMAP_PAF_DATA_END_UNKNOWN,
    IMAP_PAF_DATA_END_PAREN
} ImapDataEnd;

/* State tracker for IMAP PAF */
struct ImapPafData
{
    MimeDataPafInfo mime_info;    /* Mime response information */
    ImapPafState imap_state;      /* The current IMAP paf stat */
    ImapDataInfo imap_data_info;  /* Used for parsing data */
    ImapDataEnd data_end_state;
    bool end_of_data;
};

class ImapSplitter : public StreamSplitter
{
public:
    ImapSplitter(bool c2s);
    ~ImapSplitter();

    Status scan(Flow*, const uint8_t* data, uint32_t len,
        uint32_t flags, uint32_t* fp) override;

    virtual bool is_paf() override { return true; }

public:
    ImapPafData state;
};

bool imap_is_data_end(void* ssn);

#endif

