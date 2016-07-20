#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include "stinger_core/stinger.h"
#include "stinger_core/stinger_atomics.h"
#include "stinger_core/xmalloc.h"
#include "stinger_core/stinger_error.h"
#include "stinger_net/stinger_alg.h"
extern "C" {
#include "stinger_alg/pagerank.h"
}
#include "streaming_pagerank.h"

using namespace gt::stinger;

std::string
StreamingPagerank::getName() { return "pagerank"; }

int64_t
StreamingPagerank::getDataPerVertex() { return sizeof(double); }

std::string
StreamingPagerank::getDataDescription() { return "d pagerank"; }

StreamingPagerank::StreamingPagerank(
    const char * type_str,
    int type_specified,
    int directed,
    double epsilon,
    double dampingfactor,
    int64_t maxiter)
{
    this->type_str = type_str;
    this->type_specified = type_specified;
    this->directed = directed;
    this->epsilon = epsilon;
    this->dampingfactor = dampingfactor;
    this->maxiter = maxiter;
}

void
StreamingPagerank::onInit(stinger_registered_alg * alg)
{
    pr = (double *)alg->alg_data;
    OMP("omp parallel for")
    for(uint64_t v = 0; v < alg->stinger->max_nv; v++) {
      pr[v] = 1 / ((double)alg->stinger->max_nv);
    }

    tmp_pr = (double *)xcalloc(alg->stinger->max_nv, sizeof(double));

    int64_t type = -1;
    if(type_specified) {
      type = stinger_etype_names_lookup_type(alg->stinger, type_str);
    }
    if(type_specified && type > -1) {
      page_rank_type(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter, type);
    } else if (!type_specified) {
      page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter);
    }
}

void
StreamingPagerank::onPre(stinger_registered_alg * alg)
{
    /* nothing to do */
}

void
StreamingPagerank::onPost(stinger_registered_alg * alg)
{
    int64_t type = -1;
    if(type_specified) {
      type = stinger_etype_names_lookup_type(alg->stinger, type_str);
      if(type > -1) {
        page_rank_type(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter, type);
      } else {
        LOG_W_A("TYPE DOES NOT EXIST %s", type_str);
        LOG_W("Existing types:");
        // TODO: Don't go through the loop if LOG_W isn't enabled
        for(int64_t t = 0; t < stinger_etype_names_count(alg->stinger); t++) {
          LOG_W_A("  > %ld %s", (long) t, stinger_etype_names_lookup_name(alg->stinger, t));
        }
      }
    } else {
      page_rank(alg->stinger, stinger_mapping_nv(alg->stinger), pr, tmp_pr, epsilon, dampingfactor, maxiter);
    }
}

StreamingPagerank::~StreamingPagerank()
{
    xfree(tmp_pr);
}
