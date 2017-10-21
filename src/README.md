# CoAP for constrained devices, for real

# TODO:

3-tier handle lookup system:

0.  Direct access, no redirection (may be of little to limited value)
1.  Index access, one level of redirection (direct array lookup redirects to specific memory page)
2.  Compact access, two level of redirection (search thru list for ID match on handle, which then maps to a specific memory page)

Also, memory pages themselves are used to store the handle lookups, so in theory the aforementioned approaches may be mixed and matched.  Possibly even across a spread of different handles, though this would potentially introduce significant slowness without a DB-like indexing technique

Each memory page might have the opportunity to select which system they use, in which case a begin-end of handle designation may be more important.  However this is starting to get complicated

More simplistic would be a tier 0 can only be followed by a tier 1 and a tier 1 can only be followed by a tier 2

However, allowing a tier 1 to follow a tier 2 could have significant advantages.  