namespace tags2 {

struct notifier_context {};

}


template <class TRegistrar>
struct NotifierContext : tags2::notifier_context
{
    typedef embr::coap::internal::NotifyHelperBase<TRegistrar> notifier_type;

    notifier_type& notifier_;

    notifier_type& notifier() const { return notifier_; }

    NotifierContext(notifier_type& n) : notifier_(n) {}
};
