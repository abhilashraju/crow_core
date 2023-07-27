http_response_test_sources = [
    'http_response_test.cpp'
]

http_response_test_deps = [
json_dep,boost_dep
]



http_response_test = executable('http_response_test', 
    [http_response_test_sources, test_main], 
    include_directories : core_includes,
    dependencies : [http_response_test_deps,test_deps,openssl_dep], 
    cpp_args : CROW_ARGS,
    link_with : [ test_dep_libs])

test('response test', http_response_test)

all_test_deps += http_response_test_deps
# all_test_dep_libs += http_response_test_lib_deps
# project_benchmark_sources += files(http_response_test_sources)