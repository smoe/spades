/**
 * condensed_graph.cpp
 *
 *  Created on: Feb 21, 2011
 *      Author: sergey
 */
#include "condensedGraph.hpp"
#include "logging.hpp"
#include "graphVisualizer.hpp"
#include <set>
#include <iostream>
#include <tr1/unordered_map>

using namespace std;

namespace condensed_graph {

void Graph::FixIncomingOnSplit(Vertex* v, Vertex* v1, Vertex* v2) {
	vector<Vertex*> anc = Anc(v);
	for (size_t i = 0; i < anc.size(); ++i) {
		Vertex* ancestor = anc[i];
		if (ancestor == v->complement()) {
			LinkVertices(v1 -> complement(), v1);
		} else if (ancestor == v) {
			LinkVertices(v2, v1);
		} else {
			//trivial case
			LinkVertices(ancestor, v1);
		}
	}
}

void Graph::FixIncomingOnMerge(Vertex* v1, Vertex* v2, Vertex* v) {
	vector<Vertex*> anc = Anc(v1);
	for (size_t i = 0; i < anc.size(); ++i) {
		Vertex* ancestor = anc[i];
		if (ancestor == v1->complement()) {
			LinkVertices(v->complement(), v);
		} else if (ancestor == v2) {
			LinkVertices(v, v);
		} else {
			//trivial case
			LinkVertices(ancestor, v);
		}
	}
}

bool Graph::CanBeDeleted(Vertex* v) const {
	vector<Vertex*> anc = Anc(v);
	for (size_t i = 0; i < anc.size(); ++i) {
		Vertex* ancestor = anc[i];
		if (ancestor != v && ancestor != v->complement()) {
			for (size_t j = 0; j < 4; ++j) {
				if (ancestor->desc()[j] == v) {
					return false;
				}
			}
		}
	}
	return true;
}

vector<Vertex*> Graph::Anc(const Vertex* v) const {
	vector<Vertex*> ans;
	Vertex* const * compl_desc = v->complement()->desc();
	for (int i = 3; i >= 0; --i) {
		if (compl_desc[i] != NULL) {
			ans.push_back(compl_desc[i]->complement());
		}
	}
	return ans;
}

vector<Vertex*> Graph::Desc(const Vertex* v) const {
	vector<Vertex*> ans;
	Vertex* const * desc = v->desc();
	for (int i = 0; i < 4; ++i) {
		if (desc[i] != NULL) {
			ans.push_back(desc[i]);
		}
	}
	return ans;
}

Vertex* Graph::AddVertex(const Sequence &nucls) {
	DEBUG("Adding vertex for sequence '" << nucls.str() << "' and its complement '" << (!nucls).str() << "'");
	Vertex* v1 = new Vertex(nucls);
	Vertex* v2 = new Vertex(!nucls);
	v1->set_complement(v2);
	v2->set_complement(v1);
	vertices_.insert(v1);
	vertices_.insert(v2);

	action_handler_->HandleAdd(v1);
	return v1;
}

void Graph::DeleteVertex(Vertex* v) {
	DEBUG("Deleting vertex '" << v->nucls().str() << "' and its complement '" << v->complement()->nucls().str() << "'")
	assert(CanBeDeleted(v));
	assert(CanBeDeleted(v->complement()));

	Vertex* complement = v->complement();
	vertices_.erase(v);
	vertices_.erase(complement);

	action_handler_->HandleDelete(v);

	delete v;
	delete complement;
}

Vertex* Graph::SplitVertex(Vertex* v, size_t pos) {
	DEBUG("Splitting vertex '" << v->nucls().str() <<"' of size " << v->size() << " at position "<< pos);
	assert(pos <= v->size());

	if (pos == v->size()) {
		return v;
	};

	Sequence nucls = v->nucls();

	Vertex* v1 = AddVertex(nucls.Subseq(0, pos));
	Vertex* v2 = AddVertex(nucls.Subseq(pos - (k_ - 1), nucls.size())); // nucls.size() can be omitted here

	LinkVertices(v1, v2);

	FixIncomingOnSplit(v, v1, v2);

	FixIncomingOnSplit(v->complement(), v2->complement(),
			v1->complement());

	action_handler_->HandleSplit(v, pos, v1, v2);

	DeleteVertex(v);

	return v1;
}

Vertex* Graph::Merge(Vertex* v1, Vertex* v2) {
	DEBUG("Merging vertices '" << v1->nucls().str() << "' and '" << v2->nucls().str() << "' and their complement")
	assert(IsMergePossible(v1, v2));

	Vertex* v = AddVertex(v1->nucls() + v2->nucls().Subseq(k_ - 1));
	FixIncomingOnMerge(v1, v2, v);
	FixIncomingOnMerge(v2->complement(), v1->complement(),
			v->complement());

	action_handler_->HandleMerge(v1, v2, v);

	DeleteVertex(v1);
	DeleteVertex(v2);
	return v;
}

void Graph::AddDesc(Vertex* anc, Vertex* desc) {
	anc->AddDesc(desc, desc->nucls()[k_ - 1]);
}

void Graph::LinkVertices(Vertex* anc, Vertex* desc) {
	DEBUG("Linking vertices '" << anc->nucls().str() << "' and '"<< desc->nucls().str() <<"' and their complement")
	assert(AreLinkable(anc, desc));

	AddDesc(anc, desc);
	AddDesc(desc->complement(), anc->complement());
}

void DFS::go(Vertex* v, vector<Vertex*>& stack, Handler& h) {
	if (visited_.count(v) == 0) {
		h.HandleStartVertex(v);
		visited_.insert(v);
		vector<Vertex*> desc = g_.Desc(v);
		for (size_t i = 0; i < desc.size(); ++i) {
			Vertex* descendent = desc[i];
			h.HandleEdge(v, descendent);
			stack.push_back(descendent);
		}
	}
}

void DFS::Traverse(Handler& h) {
	for (set<Vertex*>::iterator it = g_.vertices().begin(); it
			!= g_.vertices().end(); it++) {
		vector<Vertex*> stack;
		stack.push_back(*it);
		while (!stack.empty()) {
			go(stack[stack.size() - 1], stack, h);
			stack.pop_back();
		}
	}
}

void SimpleGraphVisualizer::Visualize(const Graph& g) {
	VisHandler h(gp_);
	DFS(g).Traverse(h);
	gp_.output();
}

}

