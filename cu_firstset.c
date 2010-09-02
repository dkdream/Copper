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

/* */

struct meta_blob {
    struct prs_metafirst meta;
    struct prs_firstset  first;
};

static bool meta_ComputeSets(PrsInput input, PrsNode node, PrsMetaFirst *target)
{
    PrsMetaFirst result = 0;

    inline isTransparent(PrsNode value) {
        assert(value);

        if (value->oper == prs_Begin)     return true;
        if (value->oper == prs_End)       return true;
        if (value->oper == prs_Predicate) return true;
        if (value->oper == prs_Thunk)     return true;

        return value->type == pft_transparent;
    }

    inline bool allocate(bool bits) {
        if (node->metadata) {
            *target = result = node->metadata;
            return true;
        }

        if (!bits) {
            unsigned fullsize = sizeof(struct prs_metafirst);

             *target = result = malloc(fullsize);

            if (!result) return false;

            memset(result, 0, fullsize);

            result->done   = false;
            result->node   = node;
            node->metadata = first;
            return true;
        }


        unsigned fullsize = sizeof(struct meta_blob);

        struct meta_blob* temp = malloc(fullsize);

        if (!temp) return false;

        *target = result = (PrsMetaFirst) temp;

        memset(temp, 0, fullsize);

        PrsFirstSet set = &temp->first;

        result->done   = false;
        result->node   = node;
        result->first  = set;
        node->metadata = first;

        if (node->first) {
            unsigned char *src  = node->first->bitfield;
            unsigned char *bits = set->bitfield;
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
        assert(first);
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
        assert(first);
        assert(result->first);

        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= (src[inx] ^ 255);
        }
    }

    //----

    // @name - an named event
    // set state.begin
    // set state.begin
    // %predicate
    // {...} - an unnamed event
    inline bool do_Transparent() {
        if (allocate(false, 0, 0)) return false;
        result->done = true;
        return true;
    }

    // e !
    inline bool do_AssertFalse() {
        PrsMetaFirst child;

        if (!allocate(true)) return false;

        if (!meta_ComputeSets(input, node->arg.node, &child)) return false;

        remove(child);

        return true;
    }

    inline bool do_AssertTrue() {
        PrsMetaFirst child;

        if (!allocate(true)) return false;
        if (!meta_ComputeSets(input, node->arg.node, &child)) return false;

        merge(child);

        return true;
    }

    // e1 e2 /
    inline bool do_Choice() {
        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!allocate(true)) return false;
        if (!meta_ComputeSets(input, node->arg.pair->left,  &left))  return false;
        if (!meta_ComputeSets(input, node->arg.pair->right, &right)) return false;

        merge(left);
        merge(right);

        if (left->done) {
            if (right->done) {
                result->done;
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
        if (!allocate(false)) return false;

        result->done  = true;
        result->first = node->first;

        return true;
    }

    // name
    inline bool do_MatchName() {
        const char *name = node->arg.name;
        PrsNode     value;

        if (!allocate(true)) return false;

        if (!node(name, &value)) return false;

        if (!meta_ComputeSets(input, value, &child)) return false;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e +
    // e *
    // e ?
    inline bool do_CopyChild() {
        PrsMetaFirst child;

        if (!allocate(true)) return false;

        if (!meta_ComputeSets(input, node->arg.node, &child)) return false;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e1 e2 ;
    inline bool do_Sequence() {
        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!allocate(true)) return false;

        if (!meta_ComputeSets(input, node->arg.pair->left,  &left))  return false;
        if (!meta_ComputeSets(input, node->arg.pair->right, &right)) return false;

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
    if (!target) return false;
    if (!node)   return false;

    if (node->metadata) {
        *target = node->metadata;
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
}

static bool meta_Recheck(PrsInput input, PrsNode node, PrsMetaFirst *target)
{
    PrsMetaFirst result = 0;

    inline isTransparent(PrsNode value) {
        assert(value);

        if (value->oper == prs_Begin)     return true;
        if (value->oper == prs_End)       return true;
        if (value->oper == prs_Predicate) return true;
        if (value->oper == prs_Thunk)     return true;

        return value->type == pft_transparent;
    }

    inline void merge(PrsMetaFirst from) {
        assert(from);
        assert(first);
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
        assert(first);
        assert(result->first);

        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= (src[inx] ^ 255);
        }
    }

    //----

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

        if (!allocate(true)) return false;
        if (!meta_Recheck(input, node->arg.pair->left,  &left))  return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right)) return false;

        merge(left);
        merge(right);

        if (left->done) {
            if (right->done) {
                result->done;
            }
        }

        return true;
    }


    // name
    inline bool do_MatchName() {
        const char *name = node->arg.name;
        PrsNode     value;

        if (!allocate(true)) return false;

        if (!node(name, &value)) return false;

        if (!meta_Recheck(input, value, &child)) return false;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e +
    // e *
    // e ?
    inline bool do_CopyChild() {
        PrsMetaFirst child;

        if (!allocate(true)) return false;

        if (!meta_Recheck(input, node->arg.node, &child)) return false;

        merge(child);

        result->done = child->done;

        return true;
    }

    // e1 e2 ;
    inline bool do_Sequence() {
        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!allocate(true)) return false;

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
    if (!target) return false;
    if (!node)   return false;

    *target = result = node->metadata;

    if (result->done) return true;

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

}
