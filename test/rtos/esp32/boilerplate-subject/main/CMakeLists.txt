set(COMPONENT_SRCS "main.cpp" "app.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_options("-fstack-usage" "-Wstack-usage=128")

#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")
#component_compile_options("-Wstack-usage=64" "-S")