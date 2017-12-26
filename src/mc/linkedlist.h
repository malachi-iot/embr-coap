//
// Created by Malachi Burke on 12/26/17.
//
// TODO: bring in fact.embedded or ETL version instead.  This is just temporary
// since both of those are large and unproven in the Blackfin/VDK environment

#ifndef MC_COAP_TEST_LINKEDLIST_H
#define MC_COAP_TEST_LINKEDLIST_H

namespace moducom { namespace experimental {

template <class TNode>
class forward_node
{
    TNode* _next;

protected:

    void next(TNode* _next) { this->_next = _next; }

public:
    TNode* next() const { return  _next; }
};

template <class TNode>
class forward_list
{
    TNode* _head;

public:
    void head(TNode* _head) { this->_head = _head; }
    TNode* head() const { return _head; }
};

}}

#endif //MC_COAP_TEST_LINKEDLIST_H
