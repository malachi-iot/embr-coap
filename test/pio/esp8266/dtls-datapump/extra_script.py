Import('env')

#
# Dump build environment (for debug)
# print env.Dump()
#

# verified this DOES append mbedtls to the linker script, yet we still have
# unresolved references
env.Append(
  LINKFLAGS=[
      "-lmbedtls",
      "-Wl,-lmbedtls"
  ]
)