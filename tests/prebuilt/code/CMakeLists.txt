# set(COMPONENT_REQUIRES code)
# set(COMPONENT_PRIV_REQUIRES )

idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES prebuilt esptool_py
                    INCLUDE_DIRS "")
