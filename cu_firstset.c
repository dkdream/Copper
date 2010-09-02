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

/* */

struct meta_blob {
    struct prs_metafirst meta;
    struct prs_metaset   first;
};

static bool meta_StartFirstSets(PrsInput input, PrsNode node, PrsMetaFirst *target)
{
    PrsMetaFirst result = 0;

    inline bool isTransparent(PrsNode value) {
        assert(value);

        if (value->oper == prs_Begin)     return true;
        if (value->oper == prs_End)       return true;
        if (value->oper == prs_Predicate) return true;
        if (value->oper == prs_Thunk)     return true;

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

        if (left->done) {
            if (right->done) {
                result->done = true;
            }
        }

        return true;
    }

    // 'chr
    // .
    // begin-end
    // [...]
    // "..."
    inline bool do_CopyNode() {
        if (!allocate(true)) return false;
        result->done  = true;
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

        result->done = child->done;

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

        result->done = child->done;

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

        if (!isTransparent(node->arg.pair->left)) {
            merge(left);
            result->done = left->done;
        } else {
            merge(left);
            merge(right);
            result->done = left->done && right->done;
        }

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

static bool meta_Recheck(PrsInput input, PrsNode node, PrsMetaFirst *target)
{
    PrsMetaFirst result = 0;

    inline bool isTransparent(PrsNode value) {
        assert(value);

        if (value->oper == prs_Begin)     return true;
        if (value->oper == prs_End)       return true;
        if (value->oper == prs_Predicate) return true;
        if (value->oper == prs_Thunk)     return true;

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
        return true;
    }

    // e !
    inline bool do_AssertFalse() {
        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child)) return false;

        remove(child);

        result->done = child->done;

        return true;
    }

    // e &
    inline bool do_AssertTrue() {
        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child)) return false;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e1 e2 /
    inline bool do_Choice() {
        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_Recheck(input, node->arg.pair->left,  &left))  return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right)) return false;

        merge(left);
        merge(right);

        if (left->done) {
            if (right->done) {
                result->done = true;
            }
        }

        return true;
    }

    // name
    inline bool do_MatchName() {
        const char *name = node->arg.name;
        PrsNode     value;

        if (!input->node(input, name, &value)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }

        PrsMetaFirst child = value->metadata;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e +
    // e *
    // e ?
    inline bool do_CopyChild() {
        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child)) return false;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e1 e2 ;
    inline bool do_Sequence() {
        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_Recheck(input, node->arg.pair->left,  &left))  return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right)) return false;

        if (!isTransparent(node->arg.pair->left)) {
            merge(left);
            result->done = left->done;
        } else {
            merge(left);
            merge(right);
            result->done = left->done && right->done;
        }

        return true;
    }

    if (!input)  return false;
    if (!node)   return false;

    result = node->metadata;

    if (target) *target = result;

    if (result->done) return true;

    CU_DEBUG(1, "meta_Recheck checking %s\n", oper2name(node->oper));

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

    if (node->metadata) return true;

    if (!meta_StartFirstSets(input, node, 0)) {
        CU_DEBUG(1, "meta_StartFirstSets checking %s failed\n", oper2name(node->oper));
        return false;
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

static bool tree_MergeFirstSets(PrsInput input, PrsTree tree, bool *done) {
    struct prs_metaset holding;
    PrsMetaFirst metadata = 0;

    inline void copy() {
        unsigned char *src    = metadata->first->bitfield;
        unsigned char *bits   = holding.bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] = src[inx];
        }
    }

    inline bool match() {
        unsigned char *src    = metadata->first->bitfield;
        unsigned char *bits   = holding.bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            if (bits[inx] != src[inx]) {
                CU_DEBUG(1, "node_%x metadata changed\n", (unsigned) tree->node);
                return false;
            }
        }
        return true;
    }

    if (!input)      return false;
    if (!done)       return false;
    if (!tree)       return true;
    if (!tree->node) return false;

    PrsNode node = tree->node;

    if (!node->metadata) return false;

    metadata = node->metadata;

    if (!metadata->first) {
        switch (node->oper) {
        case prs_Apply:       return true;
        case prs_Begin:       return true;
        case prs_End:         return true;
        case prs_Predicate:   return true;
        case prs_Thunk:       return true;
        default:
            return false;
        }
    }

    copy();

    if (!meta_Recheck(input, node, 0)) return false;

    if (match()) *done = false;

    if (!tree_MergeFirstSets(input, tree->left, done)) return false;

    return tree_MergeFirstSets(input, tree->right, done);
}


static bool tree_MarkDone(PrsInput input, PrsTree tree) {
    if (!input)      return false;
    if (!tree)       return true;
    if (!tree->node) return false;

    PrsNode node = tree->node;

    if (!node->metadata) return false;

    PrsMetaFirst metadata = node->metadata;

    metadata->done = true;

    if (!tree_MarkDone(input, tree->left)) return false;

    return tree_MarkDone(input, tree->right);
}

extern bool cu_FillMetadata(PrsInput input) {
    if (!input) return false;
    if (!input->map) return false;

    PrsTree root = input->map;

    CU_DEBUG(1, "starting meta first sets\n");
    if (!tree_StartFirstSets(input, root)) return false;

    bool done = false;

    for ( ; !done ; ) {
        done = true;
        CU_DEBUG(1, "merging meta first sets\n");
        if (!tree_MergeFirstSets(input, root, &done)) return false;
    }

    CU_DEBUG(1, "marking meta first sets\n");
    if (!tree_MarkDone(input, root)) return false;

    CU_DEBUG(1, "finalising meta first sets\n");
    if (!tree_MergeFirstSets(input, root, &done)) return false;

    return done;

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



