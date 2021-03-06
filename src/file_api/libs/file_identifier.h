//--------------------------------------------------------------------------
// Copyright (C) 2014-2015 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2012-2013 Sourcefire, Inc.
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
/*
**  Author(s):  Hui Cao <huica@cisco.com>
**
**  NOTES
**  5.25.2012 - Initial Source Code. Hui Cao
*/

#ifndef FILE_IDENTIFIER_H
#define FILE_IDENTIFIER_H
#include "file_lib.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "sfghash.h"
#include <list>

#define FILE_ID_MAX          1024

#define MAX_BRANCH (UINT8_MAX + 1)

enum IdNodeState
{
    ID_NODE_NEW,
    ID_NODE_USED,
    ID_NODE_SHARED
} ;

class FileMagicData
{
public:
    void clear(void);
    std::string content_str;   /* magic content to match*/
    std::string content;       /* magic content raw values*/
    uint32_t offset;           /* pattern search start offset */
    bool operator < (const FileMagicData& magic) const
    {
        return (offset < magic.offset);
    }
};

typedef std::vector<FileMagicData> FileMagics;

class FileMagicRule
{
public:
    void clear(void);
    uint32_t rev = 0;
    uint32_t id = 0;
    std::string message;
    std::string type;
    std::string category;
    std::string version;
    FileMagics file_magics;
};

typedef struct _IdentifierNode
{
    uint32_t type_id;       /* magic content to match*/
    IdNodeState state;
    uint32_t offset;            /* offset from file start */
    struct _IdentifierNode* next[MAX_BRANCH]; /* pointer to an array of 256 identifiers pointers*/
} IdentifierNode;

struct IDMemoryBlock
{
    void *mem;
};

typedef std::list<IDMemoryBlock >  IDMemoryBlocks;

class FileIdenfifier
{
public:
    ~FileIdenfifier();
    uint32_t memory_usage(void) {return memory_used;};
    void insert_file_rule(FileMagicRule& rule);
    uint32_t find_file_type_id(uint8_t* buf, int len, FileContext* context);
    FileMagicRule* get_rule_from_id(uint32_t);
private:
    void identifierMergeHashInit(void);
    void* calloc_mem(size_t size);
    void set_node_state_shared(IdentifierNode* start);
    IdentifierNode* clone_node(IdentifierNode* start);
    void verify_magic_offset(FileMagicData* parent, FileMagicData* current);
    bool updateNext(IdentifierNode* start, IdentifierNode** next_ptr, IdentifierNode* append);
    IdentifierNode* create_trie_from_magic(FileMagicRule& rule, uint32_t type_id);
    void update_trie(IdentifierNode* start, IdentifierNode* append);

    /*properties*/
    IdentifierNode* identifier_root = NULL; /*Root of magic tries*/
    uint32_t memory_used = 0; /*Track memory usage*/
    SFGHASH* identifier_merge_hash = NULL;
    FileMagicRule file_magic_rules[FILE_ID_MAX + 1];
    IDMemoryBlocks idMemoryBlocks;
};

#endif

