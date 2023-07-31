http_connection_test_sources = [
    'http_connection_test.cpp',
    '../../boost_url.cpp',
    '../../json_html_serializer.cpp'
]

http_connection_test_deps = [
json_dep,boost_dep
]



http_connection_test = executable('http_connection_test', 
    [http_connection_test_sources, test_main], 
    include_directories : core_includes,
    dependencies : [http_connection_test_deps,test_deps,openssl_dep], 
    cpp_args : CROW_ARGS,
    link_with : [ test_dep_libs],
    link_args: ['-L' + boost_lib_path, '-lboost_coroutine'])

test('connection test', http_connection_test)

all_test_deps += http_connection_test_deps
# all_test_dep_libs += http_connection_test_lib_deps
# project_benchmark_sources += files(http_response_test_sources)