/*-*- mode: c;-*-*/
/**********************************************************************
This file is part of Copper.

Copper is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Copper is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Copper.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/
#if !defined(_copper_inline_h_)
#define _copper_inline_h_

static inline const char* oper2name(const CuOperator oper) {
    switch (oper) {
    case cu_Apply:       return "cu_Apply";       // @name - an named event
    case cu_AssertFalse: return "cu_AssertFalse"; // e !
    case cu_AssertTrue:  return "cu_AssertTrue";  // e &
    case cu_Begin:       return "cu_Begin";       // set state.begin
    case cu_Choice:      return "cu_Choice";      // e1 / e2
    case cu_End:         return "cu_End";         // set state.end
    case cu_Loop:        return "cu_Loop";        // e1 ; e2
    case cu_MatchChar:   return "cu_MatchChar";   // 'chr
    case cu_MatchDot:    return "cu_MatchDot";    // dot
    case cu_MatchName:   return "cu_MatchName";   // name
    case cu_MatchRange:  return "cu_MatchRange";  // begin-end
    case cu_MatchSet:    return "cu_MatchSet";    // [...]
    case cu_MatchText:   return "cu_MatchText";   // "..."
    case cu_OneOrMore:   return "cu_OneOrMore";   // e +
    case cu_Predicate:   return "cu_Predicate";   // %predicate
    case cu_Sequence:    return "cu_Sequence";    // e1 e2
    case cu_ZeroOrMore:  return "cu_ZeroOrMore";  // e *
    case cu_ZeroOrOne:   return "cu_ZeroOrOne";   // e ?
    case cu_Void:        break; // -nothing-
    }
    return "prs-unknown";
}

static inline const char* first2name(const CuFirstType first) {
    switch (first) {
    case pft_event:       return "pft_event";
    case pft_opaque:      return "pft_opaque";
    case pft_transparent: return "pft_transparent";
    case pft_fixed:       return "pft_fixed";
    }
    return "pft_opaque";
}

static inline CuFirstType inWithout(const CuFirstType child)
{
    // T(f!) = e, T(o&) = e, T(t!) = E, T(e!) = e
    return pft_event;
    (void) child; // remove unused parameter warning
}

static inline CuFirstType inRequired(const CuFirstType child)
{
    // T(f&) = f, T(o&) = o, T(t&) = t, T(e&) = e
    // T(f+) = f, T(o+) = o, T(t+) = t, T(e+) = ERROR
    return child;
}

static inline CuFirstType inOptional(const CuFirstType child)
{
    // T(f?) = t, T(o?) = o, T(t?) = t, T(e?) = e
    // T(f*) = t, T(o*) = o, T(t*) = t, T(e+) = ERROR

    switch (child) {
    case pft_fixed:       return pft_transparent;
    case pft_transparent: return pft_transparent;
    case pft_opaque:      return pft_opaque;
    case pft_event:       return pft_event;
    }
    return pft_opaque;
}

static inline CuFirstType inChoice(const CuFirstType before,
                                   const CuFirstType after)
{
    // T(ff/) = f, T(fo/) = o, T(ft/) = t, T(fe/) = e
    // T(of/) = o, T(oo/) = o, T(ot/) = t, T(oe/) = o
    // T(tf/) = t, T(to/) = t, T(tt/) = t, T(te/) = t
    // T(ef/) = e, T(eo/) = o, T(et/) = t, T(ee/) = e

    if (pft_transparent == before) return pft_transparent;
    if (pft_transparent == after)  return pft_transparent;
    if (pft_opaque == before)      return pft_opaque;
    if (pft_opaque == after)       return pft_opaque;
    if (pft_event == before)       return pft_event;
    if (pft_event == after)        return pft_event;

    return pft_fixed;
}

static inline CuFirstType inSequence(const CuFirstType before,
                                     const CuFirstType after)
{
    // T(ff;) = f, T(fo;) = f, T(ft;) = f, T(fe;) = f
    // T(of;) = o, T(oo;) = o, T(ot;) = o, T(oe;) = o
    // T(tf;) = f, T(to;) = o, T(tt;) = t, T(te;) = e
    // T(ef;) = f, T(eo;) = o, T(et;) = t, T(ee;) = e

    switch (before) {
    case pft_transparent:
        return after;

    case pft_event:
        if (pft_transparent == after) return before;
        return after;

    case pft_opaque:
    case pft_fixed:
        return before;
    }

    return pft_opaque;
}

static inline void cu_noop() __attribute__((always_inline));
static inline void cu_noop() { return; }

#if 1
#define CU_DEBUG(level, args...) ({ typeof (level) hold__ = (level); if (hold__ <= cu_global_debug) cu_debug(__FILE__,  __LINE__, args); })
#else
#define CU_DEBUG(level, args...) cu_noop()
#endif

#if 1
#define CU_ON_DEBUG(level, arg) ({ typeof (level) hold__ = (level); if (hold__ <= cu_global_debug) arg; })
#else
#define CU_DEBUG(level, args...) cu_noop()
#endif

#if 1
#define CU_ERROR(args...) cu_error(__FILE__,  __LINE__, args)
#else
#define CU_ERROR(args...) cu_noop()
#endif

#if 1
#define CU_ERROR_PART(args...) cu_error_part(args)
#else
#define CU_ERROR_PART(args...) cu_noop()
#endif

#if 1
#define CU_ERROR_AT(args...) cu_error(args)
#else
#define CU_ERROR_AT(args...) cu_noop()
#endif

#endif
