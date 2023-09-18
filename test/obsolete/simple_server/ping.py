#!/usr/bin/python

from coapthon.client.helperclient import HelperClient

host = "127.0.0.1"
port = 5683
path = "basic"

client = HelperClient(server=(host,port))
print "Issuing request"
response = client.get(path)
print "Got response"
print response.pretty_print()
client.stop()

