

redfish_event_sources = [
    'src/redfish_event.cpp'
]


core_includes = include_directories(['../include','.',
'../http','..',
'../plugin-ifaces'])


redfish_event_dep = declare_dependency(
	include_directories : [core_includes],
	dependencies : [dependency('threads'),openssl_dep,boost_dep,json_dep]
)
redfish_event_deps = [
json_dep,boost_dep,openssl_dep
]



redfish_event = library('redfish_event', 
    sources : redfish_event_sources,
    include_directories : core_includes,
    dependencies : [redfish_event_deps],
    cpp_args : CROW_ARGS,
    link_with : [])
    


