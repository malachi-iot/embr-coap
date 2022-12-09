# echo

(actually is known as CoAP `ping`)

This tests the following components:

* Streambuf CoAP decode/encode 

## How to test

Using any CoAP CLI makes this pretty easy.  For npm's coap-cli, one can merely type:

`coap get coap://192.168.0.0`

Substituting esp32's IP address.  If you get back a 203, you are good to go.
