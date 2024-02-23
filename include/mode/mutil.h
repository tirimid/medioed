#ifndef MODE_MUTIL_H
#define MODE_MUTIL_H

#include <stdbool.h>
#include <stddef.h>

#include "frame.h"
#include "util.h"

enum pair_flags {
	PF_PAREN = 0x1,
	PF_BRACKET = 0x2,
	PF_BRACE = 0x4,
	PF_ANGLE = 0x8,
	PF_SQUOTE = 0x10,
	PF_DQUOTE = 0x20,
};

struct mu_region {
	// lb..=ub.
	size_t lb, ub;
};

VEC_DEF_PROTO(struct mu_region, mu_region)

void mu_init(struct frame *f);
void mu_set_base(void);
void mu_set_pairing(unsigned long flags);
void mu_finish_indent(size_t ln, size_t first_ch, unsigned ntab, unsigned nspace);

// `skip` regions are expected to be in forward-linear order.
// this is the natural way they should be parsed out anyway, and allows a big
// optimization by checking only one bounds-value at a time rather than having
// to check every region when traversing a character.
// values determined relative to `pos`.
size_t mu_get_ln(size_t pos, struct vec_mu_region const *skip);
size_t mu_get_ln_end(size_t pos, struct vec_mu_region const *skip);
size_t mu_get_prev_ln(size_t pos, struct vec_mu_region const *skip);
size_t mu_get_first(size_t pos, struct vec_mu_region const *skip, bool (*is_sig)(wchar_t));
size_t mu_get_last(size_t pos, struct vec_mu_region const *skip, bool (*is_sig)(wchar_t));

#endif
