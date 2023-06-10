FILE(GLOB_RECURSE FUNCTION_source ./*.*)

idf_component_register(SRCS ${main_source} INCLUDE_DIRS "./")