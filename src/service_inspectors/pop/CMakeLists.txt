
set( FILE_LIST
    pop.cc
    pop.h
    pop_paf.cc
    pop_paf.h
    pop_config.h
    pop_module.cc
    pop_module.h
)

if (STATIC_INSPECTORS)
    add_library( pop STATIC ${FILE_LIST})

else (STATIC_INSPECTORS)
    add_shared_library(pop inspectors ${FILE_LIST})

endif (STATIC_INSPECTORS)
