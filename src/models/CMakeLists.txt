add_library(models STATIC
    treemodel.cpp
    topproxy.cpp
    data.cpp
    callercalleemodel.cpp
    costdelegate.cpp
    processmodel.cpp
    processfiltermodel.cpp
    processlist_unix.cpp
    timelinedelegate.cpp
    timeaxisheaderview.cpp
    eventmodel.cpp
    filterandzoomstack.cpp
    disassemblyoutput.cpp
    disassemblymodel.cpp
    ../settings.cpp
    ../util.cpp
    callercalleeproxy.cpp
    frequencymodel.cpp
    disassemblydelegate.cpp
)

if (${BUILD_WITH_ASAN})
    target_compile_options(models PRIVATE -fsanitize=address)
    target_link_options(models PRIVATE -fsanitize=address)
endif()

target_link_libraries(models
    Qt5::Core
    Qt5::Widgets
    KF5::ItemModels
    KF5::ConfigWidgets
    KF5::Parts
    PrefixTickLabels
)
