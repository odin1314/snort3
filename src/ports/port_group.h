//--------------------------------------------------------------------------
// Copyright (C) 2014-2015 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2002-2013 Sourcefire, Inc.
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

// port_group.h derived from pcrm.h by
/*
** Marc Norton <mnorton@sourcefire.com>
** Dan Roelker <droelker@sourcefire.com>
*/
#ifndef PortGroup_H
#define PortGroup_H

enum PmType
{
    PM_TYPE_PKT,
    PM_TYPE_ALT,
    PM_TYPE_KEY,
    PM_TYPE_HEADER,
    PM_TYPE_BODY,
    PM_TYPE_FILE,
    PM_TYPE_MAX
};

struct RULE_NODE
{
    RULE_NODE* rnNext;
    void* rnRuleData;
    int iRuleNodeID;
};

struct PortGroup
{
    // non-fast-pattern list
    RULE_NODE* nfp_head, * nfp_tail;

    // pattern matchers
    class Mpse* mpse[PM_TYPE_MAX];

    // detection option tree
    void* nfp_tree;

    unsigned rule_count;
    unsigned nfp_rule_count;

    // FIXIT-L these runtime counts are only valid with one packet thread
    unsigned match_count;
    unsigned event_count;

    void add_rule();
    bool add_nfp_rule(void*);
    void delete_nfp_rules();
};

#endif

