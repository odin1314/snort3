//--------------------------------------------------------------------------
// Copyright (C) 2014-2015 Cisco and/or its affiliates. All rights reserved.
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
// nhttp_msg_header.cc author Tom Peters <thopeter@cisco.com>

#include <string.h>
#include <sys/types.h>
#include <stdio.h>

#include "utils/util.h"
#include "detection/detection_util.h"

#include "nhttp_enum.h"
#include "nhttp_msg_request.h"
#include "nhttp_msg_header.h"

using namespace NHttpEnums;

NHttpMsgHeader::NHttpMsgHeader(const uint8_t* buffer, const uint16_t buf_size,
    NHttpFlowData* session_data_, SourceId source_id_, bool buf_owner, Flow* flow_) :
    NHttpMsgHeadShared(buffer, buf_size, session_data_, source_id_, buf_owner, flow_)
{
    transaction->set_header(this, source_id);
}

void NHttpMsgHeader::gen_events()
{
    if (get_header_count(HEAD_CONTENT_LENGTH) > 1)
        events.create_event(EVENT_MULTIPLE_CONTLEN);
}

void NHttpMsgHeader::print_section(FILE* output)
{
    NHttpMsgSection::print_message_title(output, "header");
    NHttpMsgHeadShared::print_headers(output);
    NHttpMsgSection::print_message_wrapup(output);
}

void NHttpMsgHeader::update_flow()
{
    // The following logic to determine body type is by no means the last word on this topic.
    // FIXIT-H need to distinguish methods such as POST that should have a body from those that
    // should not.
    // FIXIT-H need to support old implementations that don't use Content-Length but just
    // disconnect the connection.
    if ((source_id == SRC_SERVER) && ((status_code_num <= 199) || (status_code_num == 204) ||
        (status_code_num == 304)))
    {
        // No body allowed by RFC for these response codes
        // FIXIT-M inspect for Content-Length and Transfer-Encoding headers which should not be
        // present
        session_data->type_expected[SRC_SERVER] = SEC_STATUS;
        session_data->half_reset(SRC_SERVER);
    }
    else if ((source_id == SRC_SERVER) && (transaction->get_request() != nullptr) &&
        (transaction->get_request()->get_method_id() == METH_HEAD))
    {
        // No body allowed by RFC for response to HEAD method
        session_data->type_expected[SRC_SERVER] = SEC_STATUS;
        session_data->half_reset(SRC_SERVER);
    }
    // If there is a Transfer-Encoding header, see if the last of the encoded values is "chunked".
    // FIXIT-L do something with Transfer-Encoding header with chunked present but not last.
    else if ((get_header_value_norm(HEAD_TRANSFER_ENCODING).length > 0)                     &&
        ((*(int64_t*)(get_header_value_norm(HEAD_TRANSFER_ENCODING).start +
        (get_header_value_norm(HEAD_TRANSFER_ENCODING).length - 8))) == TRANSCODE_CHUNKED) )
    {
        // FIXIT-M inspect for Content-Length header which should not be present
        // Chunked body
        session_data->type_expected[source_id] = SEC_CHUNK;
        session_data->body_octets[source_id] = 0;
        session_data->section_size_target[source_id] = DATA_BLOCK_SIZE;
        if (session_data->file_depth_remaining[1-source_id] <= 0)
        {   // Bidirectional file processing is problematic FIXIT-M
            session_data->file_depth_remaining[source_id] = file_api->get_max_file_depth();
        }
        session_data->infractions[source_id].reset();
        session_data->events[source_id].reset();
    }
    else if ((get_header_value_norm(HEAD_CONTENT_LENGTH).length > 0) &&
        (*(int64_t*)get_header_value_norm(HEAD_CONTENT_LENGTH).start > 0))
    {
        // Regular body
        session_data->type_expected[source_id] = SEC_BODY;
        session_data->data_length[source_id] = *(int64_t*)get_header_value_norm(
            HEAD_CONTENT_LENGTH).start;
        session_data->body_octets[source_id] = 0;
        session_data->section_size_target[source_id] = DATA_BLOCK_SIZE;
        session_data->section_size_max[source_id] = FINAL_BLOCK_SIZE;
        if (session_data->file_depth_remaining[1-source_id] <= 0)
        {   // Bidirectional file processing is problematic FIXIT-M
            session_data->file_depth_remaining[source_id] = file_api->get_max_file_depth();
            if (source_id == SRC_CLIENT)
            {
                // FIXIT-L Cannot use new because file_api insists on freeing the mime_state using
                // free().
                session_data->mime_state = (MimeState*) new_calloc(1, sizeof(MimeState));
                file_api->set_mime_log_config_defauts(&mime_conf);
                session_data->mime_state->log_config = &mime_conf;
                file_api->set_mime_decode_config_defauts(&decode_conf);
                session_data->mime_state->decode_conf = &decode_conf;
                file_api->set_log_buffers(&session_data->mime_state->log_state,
                    session_data->mime_state->log_config);
            }
        }
        session_data->infractions[source_id].reset();
        session_data->events[source_id].reset();
    }
    else
    {
        // No body
        session_data->type_expected[source_id] = (source_id == SRC_CLIENT) ? SEC_REQUEST :
            SEC_STATUS;
        session_data->half_reset(source_id);
    }
    session_data->section_type[source_id] = SEC__NOTCOMPUTE;
}

