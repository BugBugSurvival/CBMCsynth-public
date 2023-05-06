//
// Created by julian on 06.05.23.
//

#include "subterm_heuristics.h"

#include <vector>

#include "problem.h"
#include "term_position.h"
#include "anti_unification.h"
#include "util.h"

#include <util/namespace.h>
#include <util/expr.h>

#include <util/expr_iterator.h>


std::vector<term_positiont> get_term_positions(const problemt &problem) {

    // Potential hyperparameters
    int lgg_max_height = 100;

    //...

    std::vector<exprt> all_subterms;
    for (const exprt &assertion: problem.assertions) {
        for (auto it = assertion.depth_begin(), itend = assertion.depth_end(); it != itend; ++it) {
            all_subterms.push_back(*it);
        }
    }

    std::size_t height = 0;
    exprt fst;
    exprt snd;

    for (int i = 0; i < all_subterms.size(); ++i) {
        const auto &x = all_subterms[i];
        for (int j = i; j < all_subterms.size(); ++j) {
            const auto &y = all_subterms[j];

            if (x.type() != y.type()) {
                continue;
            }

            if (x.type().id() != ID_integer) {
                continue;
            }

            if (is_subterm(x, y) or is_subterm(y, x)) {
                continue;
            }

            auto lgg = compute_lgg({{x, y}});
            if (!is_binder_free(lgg.first)) {
                continue;
            }
            auto lgg_height = expr_height(lgg.first);
            if (lgg_height > height and x != y and lgg_height <= lgg_max_height) {
                fst = x;
                snd = y;
                height = expr_height(lgg.first);
            }
        }
    }

    std::vector<exprt> terms{fst, snd};
    for (const auto &term: all_subterms) {
        if (term.type() != fst.type()) {
            continue;
        }
        if (!is_binder_free(term)) {
            continue;
        }

        terms.push_back(term);
        auto lgg = compute_lgg(terms);
        if (expr_height(lgg.first) < height) {
            terms.pop_back();
        }
    }

    std::vector<term_positiont> res;

    for (auto i = 0; i < problem.assertions.size(); ++i) {
        for (const auto &x: terms) {
            concat(res, get_pos_of_all_occurrences(x, problem.assertions[i], term_positiont(i)));
        }
    }


    return res;
}