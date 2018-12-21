#include "NProposingMatching.h"
#include "Vertex.h"
#include "PartnerList.h"
#include <stack>
#include <map>
#include <cassert>

NProposingMatching::NProposingMatching(const std::unique_ptr<BipartiteGraph>& G,
                                       bool A_proposing, int max_level)
    : MatchingAlgorithm(G), A_proposing_(A_proposing), max_level(max_level)
{}

struct PrefListBounds {
    // [begin, end)
    // begin is also the proposal index
    PreferenceList::SizeType begin;
    PreferenceList::SizeType end;

    PrefListBounds()
        : begin(0), end(0)
    {}

    PrefListBounds(PreferenceList::SizeType begin, PreferenceList::SizeType end)
        : begin(begin), end(end)
    {}
};

static bool isWorseThan(VertexPtr a, int a_level, int a_rank, VertexPtr b, int b_level, int b_rank) {
    if (a_level < b_level) {
        return true;
    } else if (a_level > b_level) {
        return false;
    } else if (a_rank > b_rank) {
        return true;
    } else if (a_rank < b_rank) {
        return false;
    } else { // should never happen
        return true;
    }
}

static bool isBetterThan(VertexPtr a, int a_level, int a_rank, VertexPtr b, int b_level, int b_rank) {
    return ! isWorseThan(a, a_level, a_rank, b, b_level, b_rank);
}

#include <iostream>
bool NProposingMatching::compute_matching() {
    std::stack<VertexPtr> free_list;
    //std::map<VertexPtr, int> in_queue;
    std::map<VertexPtr, int> vertex_level;
    std::map<VertexPtr, PrefListBounds> pref_list_bounds;
    const std::unique_ptr<BipartiteGraph>& G = get_graph();

    // choose the paritions from which the vertices will propose
    const auto& proposing_partition = A_proposing_ ? G->get_A_partition()
                                                   : G->get_B_partition();

    // set the level of every vertex in the proposing partition to 0
    // mark all proposing vertices free (by pushing into the free_list)
    // and vertices from the opposite partition implicitly free
    for (auto it : proposing_partition) {
        free_list.push(it.second);
        //in_queue[it.second] = 1;
        vertex_level[it.second] = 0;
        pref_list_bounds[it.second] = PrefListBounds(0, it.second->get_preference_list().size());
    }

    // there is at least one vertex in the free list
    while (not free_list.empty()) {
        // first vertex in free list 
        auto u = free_list.top();
        auto& u_pref_list = u->get_preference_list();
        free_list.pop(); // remove u from free_list
        //in_queue[u] = 0; // u is not in free list now

        // if u^l hasn't exhausted its preference list
        if (pref_list_bounds[u].begin < pref_list_bounds[u].end) {
            // highest ranked vertex to whom u not yet proposed
            auto v = u_pref_list.get(pref_list_bounds[u].begin).vertex;

            // M[v] exists
            if (M_.find(v) != M_.end() and M_[v].size() != 0) {
                // a worst partner must exist
                auto v_worst_partner = M_[v].get_least_preferred();
                auto u_in_v_pref_list = v->get_preference_list().find(u);

                if (isWorseThan(v_worst_partner.vertex, v_worst_partner.level, v_worst_partner.rank,
                                u, vertex_level[u], u_in_v_pref_list->rank))
                {
                    M_[u].add_partner(v, pref_list_bounds[u].begin, vertex_level[v]);
                    M_[v].add_partner(u, v->get_preference_list().find_index(u), vertex_level[u]);
                } else {
                    pref_list_bounds[v].begin += 1;
                    free_list.push(v);
                }
            } else {
                // match u and v
                M_[u].add_partner(v, pref_list_bounds[u].begin, vertex_level[v]);
                M_[v].add_partner(u, v->get_preference_list().find_index(u), vertex_level[u]);
            }
        } else if (vertex_level[u] < max_level) {
            vertex_level[u] += 1;
            pref_list_bounds[u].begin = 0;
            pref_list_bounds[u].end = u->get_preference_list().size();
        }
    }


/*
    // there is at least one vertex in the free list
    while (not free_list.empty()) {
        // first vertex in free list 
        auto u = free_list.top();
        auto& u_pref_list = u->get_preference_list();
        auto& u_partner_list = M_[u];
        free_list.pop(); // remove u from free_list
        in_queue[u] = 0; // u is not in free list now

        // preferences of u have not been exhausted
        if (proposal_index[u] < u_pref_list.size()) { // FIXME: this may be wrong
            // highest ranked vertex to whom u not yet proposed
            auto v = u_pref_list.get(proposal_index[u]).vertex;

//std::cout <<"propose u: " << u->get_id() << ", v: " << v->get_id() << ' ' << v->get_upper_quota() << '\n';
            // v's preference list and list of partners
            auto& v_pref_list = v->get_preference_list();
            auto& v_partner_list = M_[v];

            // u's rank on v's preference list
            auto u_rank = v_pref_list.find(u)->rank;

            // v's rank on u's preference list
            auto v_rank = u_pref_list.get(proposal_index[u]).rank;

assert( v_partner_list.size() <= v->get_upper_quota());
//std::cout << "quota u: " << v->get_id() << ' ' << v->get_upper_quota() << ' ' << v_partner_list.size() << '\n';
            if (v_partner_list.size() == v->get_upper_quota()) {
                // v's least preferred partner
                auto worst_partner = v_partner_list.get_least_preferred();

                // worst partners rank, and matched partners
                auto uc = worst_partner.partner;
                auto uc_rank = worst_partner.rank;
                auto& uc_partner_list = M_[uc];

                // does v prefer u over its worst partner?
                // this could be either because v's worst partner
                // has a lower level than u or has a higher rank than u 
                if    ((vertex_level[uc] < vertex_level[u])
                    or (vertex_level[uc] == vertex_level[u] and u_rank < uc_rank))
                {
                    // remove uc from v's matched partner list
                    v_partner_list.remove(uc);
                    // remove v from uc's matched partner list, mark uc free
                    uc_partner_list.remove(v);

//std::cout <<"match u: " << u->get_id() << ", v: " << v->get_id() << ", remove: " << uc->get_id() << '\n';
                    // add (u, v) in the matching
                    u_partner_list.add_partner(v, v_rank, vertex_level[v]);
                    v_partner_list.add_partner(u, u_rank, vertex_level[u]);

assert(in_queue[uc] == 0);
                    // push uc to free list
                    // this vertex hasn't exhausted preferences at this level
                    if (in_queue[uc] == 0) {//} and proposal_index[uc] + 1 < uc->get_preference_list().size()) {
                       // proposal_index[uc] += 1;
                        free_list.push(uc);
                        in_queue[uc] = 1;
                    }
                }
            } else {
                // accept the proposal
                u_partner_list.add_partner(v, v_rank, vertex_level[v]);
                v_partner_list.add_partner(u, u_rank, vertex_level[u]);
//std::cout <<"match u: " << u->get_id() << ", v: " << v->get_id() << '\n';
            }
            
            // add u to the free_list if it has residual capacity
            // and hasn't exhausted its preference list
            if (u->get_upper_quota() > u_partner_list.size() and in_queue[u] == 0) {// and proposal_index[u] + 1 < u->get_preference_list().size()) {
                proposal_index[u] += 1;
                free_list.push(u);
                in_queue[u] = 1;
//std::cout<<"index inc: " << u->get_id() << ' ' << vertex_level[u] << ' ' << proposal_index[u] <<  '\n';
            }
        } else if (vertex_level[u] < max_level - 1) {
            vertex_level[u] += 1;
            proposal_index[u] = 0;
            free_list.push(u);
            in_queue[u] = 1;
//std::cout << "level inc: " << u->get_id() << ' ' << vertex_level[u] << ' ' << proposal_index[u]<<'\n';
        } 
    }
    */

    return true;
}