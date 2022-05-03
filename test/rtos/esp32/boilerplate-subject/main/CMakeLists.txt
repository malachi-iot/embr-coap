set(COMPONENT_SRCS "main.cpp" "app.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

# This causes:
# "error: unrecognized command line option '-fstack-usage -Wstack-usage=128'; did you mean '--warn-stack-usage='"
component_compile_options("-fstack-usage -Wstack-usage=128")

#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")