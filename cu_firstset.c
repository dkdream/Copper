/***************************
 **
 ** Project: *current project*
 **
 ** Routine List:
 **    <routine-list-end>
 **
 **
 **
 ***/
#include "copper.h"

/* */
#include <string.h>
#include <error.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* */

struct meta_blob {
    struct prs_metafirst meta;
    struct prs_metaset   first;
};

static inline bool set_Equals(PrsMetaSet left, PrsMetaSet right) {

    unsigned char *lbits = left->bitfield;
    unsigned char *rbits = right->bitfield;

    unsigned inx = 0;
    for ( ; inx < 32 ; ++inx) {
        if (lbits[inx] != rbits[inx]) {
            return false;
        }
    }

    return true;
}

static inline bool set_Contains(PrsMetaSet contains, PrsMetaSet group) {

    unsigned char *cbits = contains->bitfield;
    unsigned char *gbits = group->bitfield;

    unsigned inx = 0;
    for ( ; inx < 32 ; ++inx) {
        if (cbits[inx] != gbits[inx]) {
            unsigned char check = cbits[inx] & gbits[inx];
            if (gbits[inx] != check) {
                return false;
            }
        }
    }

    return true;
}

static inline const char* node_label(PrsNode node) {
    char buffer[10];
    if (node->label) return node->label;
    sprintf(buffer, "%x__", (unsigned) node);
    return buffer;
}


static bool meta_StartFirstSets(PrsInput input, PrsNode node, PrsMetaFirst *target)
{
    PrsMetaFirst result = 0;

    inline bool isTransparent(PrsNode value) {
        assert(value);

        if (value->oper == prs_Begin)      return true;
        if (value->oper == prs_End)        return true;
        if (value->oper == prs_Predicate)  return true;
        if (value->oper == prs_Thunk)      return true;
        if (value->oper == prs_ZeroOrMore) return true;
        if (value->oper == prs_ZeroOrOne)  return true;
        if (value->oper == prs_MatchName) {
            const char *name = value->arg.name;
            PrsNode     test;
            if (!input->node(input, name, &test)) {
                CU_ERROR("node %s not found\n", name);
                return false;
            }
            return isTransparent(test);
        }

        return value->type == pft_transparent;
    }

    inline bool allocate(bool bits) {
        if (!bits) {
            unsigned fullsize = sizeof(struct prs_metafirst);

            result = malloc(fullsize);

            if (!result) return false;

            if (target) *target = result;

            memset(result, 0, fullsize);

            result->done   = false;
            result->node   = node;

            node->metadata = result;

            return true;
        }


        unsigned fullsize = sizeof(struct meta_blob);

        struct meta_blob* temp = malloc(fullsize);

        if (!temp) return false;

        result = (PrsMetaFirst) temp;

        if (target) *target = result;

        memset(temp, 0, fullsize);

        PrsMetaSet set = &temp->first;

        result->done   = false;
        result->node   = node;
        result->first  = set;

        node->metadata = result;

        if (node->first) {
            const unsigned char *src  = node->first->bitfield;
            unsigned char       *bits = set->bitfield;
            unsigned inx = 0;
            for ( ; inx < 32 ; ++inx) {
                bits[inx] |= src[inx];
            }
            return true;
        }

        if (node->oper == prs_AssertFalse) {
            unsigned char *bits = set->bitfield;
            unsigned inx = 0;
            for ( ; inx < 32 ; ++inx) {
                bits[inx] = 255;
            }
        }

        return true;
    }

    inline void merge(PrsMetaFirst from) {
        assert(from);
        assert(result);
        assert(result->first);

        if (!from->first) return;

        unsigned char *src  = from->first->bitfield;
        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= src[inx];
        }
    }

    inline void remove(PrsMetaFirst from) {
        assert(from);
        assert(result);
        assert(result->first);

        if (!from->first) return;

        unsigned char *src  = from->first->bitfield;
        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= (src[inx] ^ 255);
        }
    }

    //-----------------------------------------

    // @name - an named event
    // set state.begin
    // set state.begin
    // %predicate
    // {...} - an unnamed event
    inline bool do_Transparent() {
        if (!allocate(false)) return false;
        result->done = true;
        return true;
    }

    // e !
    inline bool do_AssertFalse() {
        if (!allocate(true)) return false;

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, node->arg.node, &child)) return false;

        remove(child);

        return true;
    }

    inline bool do_AssertTrue() {
        if (!allocate(true)) return false;

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, node->arg.node, &child)) return false;

        merge(child);

        return true;
    }

    // e1 e2 /
    inline bool do_Choice() {
        if (!allocate(true)) return false;

        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_StartFirstSets(input, node->arg.pair->left,  &left)) {
            CU_DEBUG(1, "meta_StartFirstSets checking left failed\n");
            return false;
        }
        if (!meta_StartFirstSets(input, node->arg.pair->right, &right)) {
            CU_DEBUG(1, "meta_StartFirstSets checking right failed\n");
            return false;
        }

        merge(left);
        merge(right);

        return true;
    }

    // 'chr
    // .
    // begin-end
    // [...]
    // "..."
    inline bool do_CopyNode() {
        if (!allocate(true)) return false;
        result->done = true;
        return true;
    }

    // name
    inline bool do_MatchName() {
        if (!allocate(true)) return false;

        const char *name = node->arg.name;
        PrsNode     value;

        if (!input->node(input, name, &value)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, value, &child)) return false;

        merge(child);

        return true;
    }

    // e +
    // e *
    // e ?
    inline bool do_CopyChild() {
        if (!allocate(true)) return false;

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, node->arg.node, &child)) {
            CU_DEBUG(1, "meta_StartFirstSets checking child failed\n");
            return false;
        }

        merge(child);

        return true;
    }

    // e1 e2 ;
    inline bool do_Sequence() {
        if (!allocate(true)) return false;

        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_StartFirstSets(input, node->arg.pair->left,  &left)) {
            CU_DEBUG(1, "meta_StartFirstSets checking left failed\n");
            return false;
        }
        if (!meta_StartFirstSets(input, node->arg.pair->right, &right)) {
            CU_DEBUG(1, "meta_StartFirstSets checking right failed\n");
            return false;
        }

        merge(left);
        merge(right);

#if 0
        if (isTransparent(node->arg.pair->left)) {
            merge(right);
        } else {
            CU_DEBUG(1, "meta_StartFirstSets %s(%s) not transparent\n", node_label(node->arg.pair->left), oper2name(node->arg.pair->left->oper));
        }
#endif

        return true;
    }

    if (!input)  return false;
    if (!node)   return false;

    if (node->metadata) {
        if (target) *target = node->metadata;
        return true;
    }

    switch (node->oper) {
    case prs_Apply:       return do_Transparent();
    case prs_AssertFalse: return do_AssertFalse();
    case prs_AssertTrue:  return do_AssertTrue();
    case prs_Begin:       return do_Transparent();
    case prs_Choice:      return do_Choice();
    case prs_End:         return do_Transparent();
    case prs_MatchChar:   return do_CopyNode();
    case prs_MatchDot:    return do_CopyNode();
    case prs_MatchName:   return do_MatchName();
    case prs_MatchRange:  return do_CopyNode();
    case prs_MatchSet:    return do_CopyNode();
    case prs_MatchText:   return do_CopyNode();
    case prs_OneOrMore:   return do_CopyChild();
    case prs_Predicate:   return do_Transparent();
    case prs_Sequence:    return do_Sequence();
    case prs_Thunk:       return do_Transparent();
    case prs_ZeroOrMore:  return do_CopyChild();
    case prs_ZeroOrOne:   return do_CopyChild();
    case prs_Void:
        break;
    }
    return false;
}

static bool meta_Recheck(PrsInput input, PrsNode node, PrsMetaFirst *target, bool *changed)
{
    struct prs_metaset holding;
    PrsMetaFirst result = 0;

    inline void copy() {
        unsigned char *src    = result->first->bitfield;
        unsigned char *bits   = holding.bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] = src[inx];
        }
    }

    inline bool match() {
        unsigned char *src    = result->first->bitfield;
        unsigned char *bits   = holding.bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            if (bits[inx] != src[inx]) {
                *changed = true;
                return false;
            }
        }
        return true;
    }

    inline bool isTransparent(PrsNode value) {
        assert(value);

        if (value->oper == prs_Begin)      return true;
        if (value->oper == prs_End)        return true;
        if (value->oper == prs_Predicate)  return true;
        if (value->oper == prs_Thunk)      return true;
        if (value->oper == prs_ZeroOrMore) return true;
        if (value->oper == prs_ZeroOrOne)  return true;
        if (value->oper == prs_MatchName) {
            const char *name = value->arg.name;
            PrsNode     test;
            if (!input->node(input, name, &test)) {
                CU_ERROR("node %s not found\n", name);
                return false;
            }
            return isTransparent(test);
        }

        return value->type == pft_transparent;
    }

    inline void merge(PrsMetaFirst from) {
        assert(from);
        assert(result);
        assert(result->first);

        if (!from->first) return;

        unsigned char *src  = from->first->bitfield;
        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= src[inx];
        }

        if (!set_Contains(result->first, from->first)) {
            CU_DEBUG(1, "meta_Recheck merge error\n");
        }
    }

    inline void remove(PrsMetaFirst from) {
        assert(from);
        assert(result);
        assert(result->first);

        if (!from->first) return;

        unsigned char *src  = from->first->bitfield;
        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= (src[inx] ^ 255);
        }
    }

    //------------------------------

    // @name - an named event
    // set state.begin
    // set state.begin
    // %predicate
    // {...} - an unnamed event
    // 'chr
    // .
    // begin-end
    // [...]
    // "..."
    inline bool do_Nothing() {
        if (!result->done) {
            CU_DEBUG(1, "meta_Recheck node(%s) not done\n", oper2name(node->oper));
        }
        return true;
    }

    // e !
    inline bool do_AssertFalse() {
        copy();

        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child, changed)) return false;

        remove(child);

        result->done = match();

        return true;
    }

    // e &
    inline bool do_AssertTrue() {
        copy();

        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child, changed)) return false;

        merge(child);

        result->done = match();

        return true;
    }

    // e1 e2 /
    inline bool do_Choice() {
        copy();

        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_Recheck(input, node->arg.pair->left,  &left,  changed)) return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right, changed)) return false;

        merge(left);
        merge(right);

        result->done = match();

        return true;
    }

    // name
    inline bool do_MatchName() {
        copy();

        const char *name = node->arg.name;
        PrsNode     value;

        if (!input->node(input, name, &value)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }

        PrsMetaFirst child = value->metadata;

        merge(child);

        result->done = match();

        return true;
    }

    // e +
    // e *
    // e ?
    inline bool do_CopyChild() {
        copy();

        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child, changed)) return false;

        merge(child);

        result->done = match();

        return true;
    }

    // e1 e2 ;
    inline bool do_Sequence() {
        copy();

        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_Recheck(input, node->arg.pair->left,  &left,  changed)) return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right, changed)) return false;

        merge(left);
        merge(right);

#if 0
        if (isTransparent(node->arg.pair->left)) {
            merge(right);
        } else {
            CU_DEBUG(1, "meta_Recheck %s(%s) not transparent\n", node_label(node->arg.pair->left), oper2name(node->arg.pair->left->oper));
        }
#endif

        result->done = match();

        return true;
    }

    if (!input)   return false;
    if (!node)    return false;
    if (!changed) return false;

    result = node->metadata;

    if (target) *target = result;

    switch (node->oper) {
    case prs_Apply:       return do_Nothing();
    case prs_AssertFalse: return do_AssertFalse();
    case prs_AssertTrue:  return do_AssertTrue();
    case prs_Begin:       return do_Nothing();
    case prs_Choice:      return do_Choice();
    case prs_End:         return do_Nothing();
    case prs_MatchChar:   return do_Nothing();
    case prs_MatchDot:    return do_Nothing();
    case prs_MatchName:   return do_MatchName();
    case prs_MatchRange:  return do_Nothing();
    case prs_MatchSet:    return do_Nothing();
    case prs_MatchText:   return do_Nothing();
    case prs_OneOrMore:   return do_CopyChild();
    case prs_Predicate:   return do_Nothing();
    case prs_Sequence:    return do_Sequence();
    case prs_Thunk:       return do_Nothing();
    case prs_ZeroOrMore:  return do_CopyChild();
    case prs_ZeroOrOne:   return do_CopyChild();
    case prs_Void:
        break;
    }
    return false;
}

static bool meta_Clear(PrsInput input, PrsNode node)
{
    inline bool do_Nothing() {
        return true;
    }

    inline bool do_Child() {
       return meta_Clear(input, node->arg.node);
    }

    inline bool do_Childern() {
        if (!meta_Clear(input, node->arg.pair->left))  return false;
        if (!meta_Clear(input, node->arg.pair->right)) return false;
        return true;
    }

    if (!node) return true;

    PrsMetaFirst metadata = node->metadata;

    if (metadata) free(metadata);

    node->metadata = 0;

    switch (node->oper) {
    case prs_Apply:       return do_Nothing();
    case prs_AssertFalse: return do_Child();
    case prs_AssertTrue:  return do_Child();
    case prs_Begin:       return do_Nothing();
    case prs_Choice:      return do_Childern();
    case prs_End:         return do_Nothing();
    case prs_MatchChar:   return do_Nothing();
    case prs_MatchDot:    return do_Nothing();
    case prs_MatchName:   return do_Nothing();
    case prs_MatchRange:  return do_Nothing();
    case prs_MatchSet:    return do_Nothing();
    case prs_MatchText:   return do_Nothing();
    case prs_OneOrMore:   return do_Child();
    case prs_Predicate:   return do_Nothing();
    case prs_Sequence:    return do_Childern();
    case prs_Thunk:       return do_Nothing();
    case prs_ZeroOrMore:  return do_Child();
    case prs_ZeroOrOne:   return do_Child();
    case prs_Void:
        break;
    }

    return false;
}

static bool tree_Clear(PrsInput input, PrsTree tree) {
    if (!tree) return true;
    tree->node = 0;
    if (!tree_Clear(input, tree->left)) return false;
    return tree_Clear(input, tree->right);
}

static bool tree_Fill(PrsInput input, PrsTree tree) {
    if (!input) return false;
    if (!tree)  return true;

    const char *name = tree->name;
    if (!input->node(input, name, &tree->node)) {
        CU_ERROR("node %s not found\n", name);
        return false;
    }

    if (!tree_Clear(input, tree->left)) return false;
    return tree_Clear(input, tree->right);
}

static bool tree_StartFirstSets(PrsInput input, PrsTree tree) {
    if (!input)      return false;
    if (!tree)       return true;
    if (!tree->node) return false;

    PrsNode node = tree->node;

    if (!meta_StartFirstSets(input, node, 0)) {
        CU_DEBUG(1, "meta_StartFirstSets checking %s failed\n", oper2name(node->oper));
        return false;
    }

    if (!node->metadata) {
        CU_DEBUG(1, "meta_StartFirstSets error: not metadata for %s\n", oper2name(node->oper));
        return true;
    }

    if (!tree_StartFirstSets(input, tree->left)) {
        CU_DEBUG(1, "tree_StartFirstSets checking left failed\n");
        return false;
    }
    if (!tree_StartFirstSets(input, tree->right)) {
        CU_DEBUG(1, "tree_StartFirstSets checking right failed\n");
        return false;
    }
    return true;
}

static bool tree_MergeFirstSets(PrsInput input, PrsTree tree, bool *changed) {
    if (!input)      return false;
    if (!tree)       return true;
    if (!tree->node) return false;
    if (!changed)    return false;

    PrsNode node = tree->node;

    if (!node->metadata) {
        CU_DEBUG(1, "tree_MergeFirstSets error: not metadata for %s\n", oper2name(node->oper));
        return false;
    }

    if (!meta_Recheck(input, node, 0, changed)) {
        CU_DEBUG(1, "meta_Recheck checking %s failed\n", oper2name(node->oper));
        return false;
    }

    if (!tree_MergeFirstSets(input, tree->left, changed)) return false;

    return tree_MergeFirstSets(input, tree->right, changed);
}


extern bool cu_FillMetadata(PrsInput input) {
    if (!input) return false;
    if (!input->map) return false;

    PrsTree root = input->map;

    if (!tree_StartFirstSets(input, root)) return false;

    bool changed = true;

    for ( ; changed ; ) {
        changed = false;
        if (!tree_MergeFirstSets(input, root, &changed)) {
            CU_DEBUG(1, "tree_MergeFirstSets error\n");
            return false;
        }
    }

    return true;

    (void) tree_Fill;
    (void) tree_StartFirstSets;
    (void) tree_MergeFirstSets;
}


extern bool cu_AddName(PrsInput input, PrsName name, PrsNode node) {
    assert(0 != input);
    assert(0 != name);
    assert(0 != node);

    inline bool fetch(PrsNode *target) {
        if (!input->node(input, name, target)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }
        return true;
    }

    inline bool allocate(PrsTree *target) {
        unsigned fullsize = sizeof(struct prs_tree);

        PrsTree result = malloc(fullsize);

        if (!result) return false;

        *target = result;

        result->name = name;

        if (!fetch(&result->node)) return false;

        return true;
    }

    if (!input->attach(input, name, node)) return false;

    PrsTree root = input->map;

    if (!root) {
        return allocate(&input->map);
    }

    for ( ; ; ) {
        int direction = strcmp(root->name, name);

        if (0 < direction) {
            if (root->left) {
                root = root->left;
                continue;
            }
            return allocate(&root->left);
        }

        if (0 > direction) {
            if (root->right) {
                root = root->right;
                continue;
            }
            return allocate(&root->right);
        }

        return fetch(&root->node);
    }
}



