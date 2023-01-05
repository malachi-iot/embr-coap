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

// message observer support code
template <class Context, class TContextTraits >
class ContextContainer
{
protected:
    Context* m_context;

public:
    typedef Context context_t;
    typedef TContextTraits context_traits_t;

    context_t& context() { return *m_context; }

    const context_t& context() const { return *m_context; }

    // FIX: Kludgey way of skipping some steps.  Strongly consider
    // dumping this if we can
    void context(context_t& context)
    {
        this->m_context = &context;
    }

};

template <class DataPump, DataPump* global_datapump>
class GlobalDatapumpOutputContext
{
    typedef typename DataPump::netbuf_t netbuf_t;
    typedef typename DataPump::addr_t addr_t;

public:
#ifdef FEATURE_EMBR_DATAPUMP_INLINE
    void send(netbuf_t&& out, const addr_t& addr_out)
    {
        global_datapump->enqueue_out(std::forward<netbuf_t>(out), addr_out);
    }
#else
    void send(netbuf_t& out, const addr_t& addr_out)
    {
        global_datapump->enqueue_out(out, addr_out);
    }
#endif
};
