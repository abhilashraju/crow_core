all_test_deps = []
all_test_dep_libs = []
project_test_sources = []

gtest_dep = gtest.get_variable('gtest_dep')
# thread_dep = dependency('threads')
test_main = files('tests_main.cc')

test_deps = [
    gtest_dep,
    crow_core_dependencies
]

test_dep_libs = [

]

subdir('http_response_test')
#subdir('http_connection_test')
subdir('base64_encoder_test')


# This executable contains all the tests
project_test_sources += test_main
all_test_deps += test_deps
all_test_dep_libs += test_dep_libs

all_testes = executable('all_tests', 
    project_test_sources, 
    include_directories : core_includes, 
    dependencies : all_test_deps, 
    link_with : all_test_dep_libs)
