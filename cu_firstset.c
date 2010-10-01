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
    static char buffer[10];
    if (node->label) return node->label;
    sprintf(buffer, "%x__", (unsigned) node);
    return buffer;
}

static bool meta_StartFirstSets(Copper input, PrsNode node, PrsMetaFirst *target)
{
    PrsMetaFirst result = 0;

    inline bool allocate(bool bits) {
        if (!bits) {
            unsigned fullsize = sizeof(struct prs_metafirst);

            result = malloc(fullsize);

            if (!result) return false;

            if (target) *target = result;

            memset(result, 0, fullsize);

            result->done   = false;
            result->type   = pft_fixed;
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
        result->type   = pft_fixed;
        result->first  = set;

        node->metadata = result;

        return true;
    }

    inline bool copy_node() {
        if (!node->first)   return true;
        if (!result->first) return false;

        const unsigned char *src  = node->first->bitfield;
        unsigned char       *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= src[inx];
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

    //-----------------------------------------

    // - %predicate
    // - dot
    inline bool do_Opaque() {
        if (!allocate(false)) return false;
        result->done = true;
        result->type = pft_opaque;
        return true;
    }

    // @name - an named event
    // set state.begin
    // set state.begin
    // {...} - an unnamed event
    inline bool do_Event() {
        if (!allocate(false)) return false;
        result->done = true;
        result->type = pft_event;
        return true;
    }

    // 'chr
    // [...]
    // "..."
    // begin-end
    inline bool do_CopyNode() {
        if (!allocate(true)) return false;
        copy_node();
        result->type = node->type;
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

        result->type = child->type;

        merge(child);

        return true;
    }

    // e !
    inline bool do_AssertFalse() {
        if (!allocate(false)) return false;

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, node->arg.node, &child)) return false;

        result->done = true;
        result->type = inWithout(child->type);

        return true;
    }

    // e &
    // e +
    inline bool do_AssertChild() {
        if (!allocate(true)) return false;

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, node->arg.node, &child)) {
            CU_DEBUG(1, "meta_StartFirstSets checking child failed\n");
            return false;
        }

        // T(f&) = f, T(o&) = o, T(t&) = t, T(e&) = e
        // T(f+) = f, T(o+) = o, T(t+) = t, T(e+) = ERROR

        result->type = inRequired(child->type);

        merge(child);

        return true;
    }

    // e ?
    // e *
    inline bool do_TestChild() {
        if (!allocate(true)) return false;

        PrsMetaFirst child;

        if (!meta_StartFirstSets(input, node->arg.node, &child)) {
            CU_DEBUG(1, "meta_StartFirstSets checking child failed\n");
            return false;
        }

        // T(f?) = t, T(o?) = o, T(t?) = t, T(e?) = e
        // T(f*) = t, T(o*) = o, T(t*) = t, T(e+) = ERROR

        result->type = inOptional(child->type);

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

        result->type = inChoice(left->type, right->type);

        merge(left);
        merge(right);

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

        result->type = inSequence(left->type, right->type);

        merge(left);

        if (pft_transparent == left->type) {
            merge(right);
        }

        if (pft_event == left->type) {
            merge(right);
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
    case prs_Apply:       return do_Event();
    case prs_AssertFalse: return do_AssertFalse();
    case prs_AssertTrue:  return do_AssertChild();
    case prs_Begin:       return do_Event();
    case prs_Choice:      return do_Choice();
    case prs_End:         return do_Event();
    case prs_MatchChar:   return do_CopyNode();
    case prs_MatchDot:    return do_Opaque();
    case prs_MatchName:   return do_MatchName();
    case prs_MatchRange:  return do_CopyNode();
    case prs_MatchSet:    return do_CopyNode();
    case prs_MatchText:   return do_CopyNode();
    case prs_OneOrMore:   return do_AssertChild();
    case prs_Predicate:   return do_Opaque();
    case prs_Sequence:    return do_Sequence();
    case prs_Thunk:       return do_Event();
    case prs_ZeroOrMore:  return do_TestChild();
    case prs_ZeroOrOne:   return do_TestChild();
    case prs_Void:
        break;
    }
    return false;
}

static bool meta_Recheck(Copper input, PrsNode node, PrsMetaFirst *target, bool *changed)
{
    struct prs_metaset holding;
    PrsMetaFirst result = 0;

    inline void hold() {
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

    inline bool merge(PrsMetaFirst from) {
        assert(from);
        assert(result);
        assert(result->first);

        if (!from->first) return false;

        unsigned char *src  = from->first->bitfield;
        unsigned char *bits = result->first->bitfield;
        unsigned inx = 0;
        for ( ; inx < 32 ; ++inx) {
            bits[inx] |= src[inx];
        }

        return match();
    }

    //------------------------------

    // %predicate
    // dot
    // @name - an named event
    // set state.begin
    // set state.begin
    // {...} - an unnamed event
    // 'chr
    // [...]
    // "..."
    // begin-end
    inline bool do_Nothing() {
        if (!result->done) {
            CU_DEBUG(1, "meta_Recheck node(%s) not done\n", oper2name(node->oper));
        }
        return true;
    }

    // name
    inline bool do_MatchName() {
        hold();

        const char *name = node->arg.name;
        PrsNode     value;

        if (!input->node(input, name, &value)) {
            CU_ERROR("node %s not found\n", name);
            return false;
        }

        PrsMetaFirst child = value->metadata;

        result->type = child->type;

        merge(child);

        result->done = match();

        return true;
    }

    // e !
    inline bool do_AssertFalse() {
         PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child, changed)) return false;

        result->type = inWithout(child->type);

        return true;
    }

    // e &
    // e +
    inline bool do_AssertChild() {
        hold();

        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child, changed)) return false;

        // T(f&) = f, T(o&) = o, T(t&) = t, T(e&) = e
        // T(f+) = f, T(o+) = o, T(t+) = t, T(e+) = ERROR

        result->type = inRequired(child->type);

        merge(child);

        result->done = match();

        return true;
    }

    // e ?
    // e *
    inline bool do_TestChild() {
        hold();

        PrsMetaFirst child;

        if (!meta_Recheck(input, node->arg.node, &child, changed)) return false;

        // T(f?) = t, T(o?) = o, T(t?) = t, T(e?) = e
        // T(f*) = t, T(o*) = o, T(t*) = t, T(e+) = ERROR

        result->type = inOptional(child->type);

        merge(child);

        result->done = match();

        return true;
    }

    // e1 e2 /
    inline bool do_Choice() {
        hold();

        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_Recheck(input, node->arg.pair->left,  &left,  changed)) return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right, changed)) return false;

        result->type = inChoice(left->type, right->type);

        merge(left);
        merge(right);

        result->done = match();

        return true;
    }

    // e1 e2 ;
    inline bool do_Sequence() {
        hold();

        PrsMetaFirst left;
        PrsMetaFirst right;

        if (!meta_Recheck(input, node->arg.pair->left,  &left,  changed)) return false;
        if (!meta_Recheck(input, node->arg.pair->right, &right, changed)) return false;

        result->type = inSequence(left->type, right->type);

        merge(left);

        if (pft_transparent == left->type) {
            merge(right);
        }

        if (pft_event == left->type) {
            merge(right);
        }

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
    case prs_AssertTrue:  return do_AssertChild();
    case prs_Begin:       return do_Nothing();
    case prs_Choice:      return do_Choice();
    case prs_End:         return do_Nothing();
    case prs_MatchChar:   return do_Nothing();
    case prs_MatchDot:    return do_Nothing();
    case prs_MatchName:   return do_MatchName();
    case prs_MatchRange:  return do_Nothing();
    case prs_MatchSet:    return do_Nothing();
    case prs_MatchText:   return do_Nothing();
    case prs_OneOrMore:   return do_AssertChild();
    case prs_Predicate:   return do_Nothing();
    case prs_Sequence:    return do_Sequence();
    case prs_Thunk:       return do_Nothing();
    case prs_ZeroOrMore:  return do_TestChild();
    case prs_ZeroOrOne:   return do_TestChild();
    case prs_Void:
        break;
    }
    return false;
}

static void meta_DebugSets(FILE *output, unsigned level, PrsNode node)
{
    inline void char_Write(unsigned char value)
    {
        switch (value) {
        case '\a': fprintf(output, "\'%s\'", "\\a");  return; /* bel */
        case '\b': fprintf(output, "\'%s\'", "\\b");  return; /* bs */
        case '\e': fprintf(output, "\'%s\'", "\\e");  return; /* esc */
        case '\f': fprintf(output, "\'%s\'", "\\f");  return; /* ff */
        case '\n': fprintf(output, "\'%s\'", "\\n");  return; /* nl */
        case '\r': fprintf(output, "\'%s\'", "\\r");  return; /* cr */
        case '\t': fprintf(output, "\'%s\'", "\\t");  return; /* ht */
        case '\v': fprintf(output, "\'%s\'", "\\v");  return; /* vt */
        case '\'': fprintf(output, "\'%s\'", "\\'");  return; /* ' */
        case '\"': fprintf(output, "\'%s\'", "\\\""); return; /* " */
        case '\\': fprintf(output, "\'%s\'", "\\\\"); return; /* \ */
        default:   fprintf(output, "\'%c\'", value);  return;
        }
    }

    inline void debug_charclass() {
        if (!node) return;

        PrsMetaFirst meta = node->metadata;

        if (!meta) return;
        if (!meta->first) {
            unsigned inx = 0;
            for ( ; inx < 32;  ++inx) {
                fprintf(output, "----");
            }
            return;
        }

        unsigned char *bits = meta->first->bitfield;

        if (!bits) {
            unsigned inx = 0;
            for ( ; inx < 32;  ++inx) {
                fprintf(output, "----");
            }
            return;
        }

         unsigned inx = 0;
         for ( ; inx < 32;  ++inx) {
             if (0 ==  bits[inx]) {
                fprintf(output, "\\---");
             } else {
                 fprintf(output, "\\%03o", bits[inx]);
             }
         }
    }

    inline void debug_type() {
        if (!node) return;

        PrsMetaFirst meta = node->metadata;

        if (!meta) return;

        fprintf(output, "%17s ", first2name(meta->type));
    }

    inline void debug_label(PrsNode cnode) {
        if (!cnode) {
            fprintf(output,"%x_ ", (unsigned) cnode);
            return;
        }

        if (node->label) {
            fprintf(output, "%s ", cnode->label);
            return;
        }

        fprintf(output,"%x_ ", (unsigned) cnode);
    }

    inline void debug_oper() {
        unsigned inx = 0;
        for ( ; inx < level;  ++inx) {
            fprintf(output, " |");
        }
        fprintf(output, "%s ", oper2name(node->oper));
        if (prs_MatchName == node->oper) {
            fprintf(output, "%s ", node->arg.name);
        }
        if (prs_MatchSet == node->oper) {
            fprintf(output, "[%s] ", node->arg.set->label);
        }
        if (prs_Sequence == node->oper) {
           debug_label(node->arg.pair->left);
           debug_label(node->arg.pair->right);
        }
        if (prs_Choice == node->oper) {
            fprintf(output, "( ");
            debug_label(node->arg.pair->left);
            debug_label(node->arg.pair->right);
            fprintf(output, ") ");
        }
    }

    inline void do_Child() {
        meta_DebugSets(output, level + 1, node->arg.node);
    }

    inline void do_Childern() {
        meta_DebugSets(output, level + 1, node->arg.pair->left);
        meta_DebugSets(output, level + 1, node->arg.pair->right);
    }

    if (!node) return;

    fprintf(output, "{ ");
    debug_charclass();
    fprintf(output, " } ");
    debug_type();
    debug_label(node);
    debug_oper();
    fprintf(output, "\n");

    switch (node->oper) {
    case prs_Apply:       return;
    case prs_AssertFalse: do_Child(); return;
    case prs_AssertTrue:  do_Child(); return;
    case prs_Begin:       return;
    case prs_Choice:      do_Childern(); return;
    case prs_End:         return;
    case prs_MatchChar:   return;
    case prs_MatchDot:    return;
    case prs_MatchName:   return;
    case prs_MatchRange:  return;
    case prs_MatchSet:    return;
    case prs_MatchText:   return;
    case prs_OneOrMore:   do_Child(); return;
    case prs_Predicate:   return;
    case prs_Sequence:    do_Childern(); return;
    case prs_Thunk:       return;
    case prs_ZeroOrMore:  do_Child(); return;
    case prs_ZeroOrOne:   do_Child(); return;
    case prs_Void:
        break;
    }

    return;
}

static bool meta_Clear(Copper input, PrsNode node)
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

static bool tree_Clear(Copper input, PrsTree tree) {
    if (!tree) return true;
    tree->node = 0;
    if (!tree_Clear(input, tree->left)) return false;
    return tree_Clear(input, tree->right);
}

static bool tree_Fill(Copper input, PrsTree tree) {
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

static bool tree_StartFirstSets(Copper input, PrsTree tree) {
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

static bool tree_MergeFirstSets(Copper input, PrsTree tree, bool *changed) {
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

static bool tree_DebugSets(Copper input, PrsTree tree) {
    if (!input)      return false;
    if (!tree)       return true;
    if (!tree->node) return false;

    if (!tree_DebugSets(input, tree->left)) return false;

    fprintf(stdout, "--- %s ---\n", tree->name);

    meta_DebugSets(stdout, 0, tree->node);

    return tree_DebugSets(input, tree->right);
}


extern bool cu_FillMetadata(Copper input) {

    if (!input) {
        CU_DEBUG(1, "cu_FillMetadata error: no input\n");
        return false;
    }
    if (!input->map) {
        CU_DEBUG(1, "cu_FillMetadata error: no map\n");
        return false;
    }

    PrsTree root = input->map;

    CU_DEBUG(1, "filling metadata %x\n", (unsigned) root);

    if (!tree_StartFirstSets(input, root)) {
        CU_DEBUG(1, "tree_StartFirstSets error\n");
        return false;
    }

    bool changed = true;

    for ( ; changed ; ) {
        changed = false;
        CU_ON_DEBUG(6, { tree_DebugSets(input, root); });
        CU_DEBUG(4, "merging metadata  %x\n", (unsigned) root);
        if (!tree_MergeFirstSets(input, root, &changed)) {
            CU_DEBUG(1, "tree_MergeFirstSets error\n");
            return false;
        }
    }

    CU_ON_DEBUG(1, { tree_DebugSets(input, root); });

    CU_DEBUG(1, "filling metadata %x done\n", (unsigned) root);

    return true;

    (void) tree_Fill;
    (void) tree_StartFirstSets;
    (void) tree_MergeFirstSets;
}


extern bool cu_AddName(Copper input, PrsName name, PrsNode node) {
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



