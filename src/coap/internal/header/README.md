# CoAP Header Support

3 different architectural approaches are here

## Legacy

This one has operated well for years.  It's relatively uncomplicated, but rather verbose as it's
all manual and has a ton of noisy use-once #defines.  Consider this a baseline to test quality against
since it's proven.  Also it's c++03 compliant.

Is relatively efficient given all operations are specifically targeted towards CoAP's network order.

## embr::bits::word (c++03) version

This one uses `embr::bits::word` which is much more elegant than legacy flavor, but suffers a minor flaw
in that it only operates on host endianness.  Swapping may be required to get things back to network order.
Not really a huge issue and likely to displace legacy at some point.  Noteworthy is legacy approach may
always be more efficient than this one.

## embr::bits::material (c++11) version

This one uses `embr::bits::material` which is theoretically the most elegant of all.  It has deep endian
awareness and is specifically designed for handling arbitrary streams of binary packed integers.  To do
all its tricks requires c++11.

# References

1. https://www.rfc-editor.org/rfc/rfc7252