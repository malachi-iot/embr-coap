#pragma once

namespace moducom { namespace coap { namespace experimental {

// revamped message observer support code

template <class TContext>
struct incoming_context_traits
{
    typedef typename TContext::addr_t addr_t;

    template <class TAddr>
    static void set_address(TContext& c, const TAddr& addr)
    {
        //c.address(addr);
    }
};

template <class TIncomingContext, class TRequestContextTraits = incoming_context_traits<TIncomingContext> >
class IncomingContextContainer
{
protected:
    TIncomingContext* m_context;

public:
    typedef TIncomingContext context_t;
    typedef TRequestContextTraits context_traits_t;

    context_t& context() { return *m_context; }

    const context_t& context() const { return *m_context; }

    // FIX: Kludgey way of skipping some steps.  Strongly consider
    // dumping this if we can
    void context(context_t& context)
    {
        this->m_context = &context;
    }

};

}}}
