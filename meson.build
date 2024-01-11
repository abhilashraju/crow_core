project(
  'crow_core',
  'cpp',
  version: '0.1',
  meson_version: '>=0.58.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ])
cxx = meson.get_compiler('cpp')
CROW_ARGS=['-DBMCWEB_ENABLE_SSL','-DBMCWEB_INSECURE_DISABLE_AUTHX','-DBOOST_URL_NO_SOURCE_LOCATION','-DBMCWEB_INSECURE_DISABLE_AUTHX']
#CHAI_ARGS=[]
openssl_dep = dependency('openssl', version: '>=1.1.1')
json_dep = dependency('nlohmann_json')
gtest = subproject('gtest')
add_project_arguments(
cxx.get_supported_arguments([
  '-DBOOST_ASIO_DISABLE_CONCEPTS',
  '-DBOOST_ALL_NO_LIB',
  '-DBOOST_ALLOW_DEPRECATED_HEADERS',
  #'-DBOOST_ASIO_DISABLE_THREADS',
  #'-DBOOST_ASIO_NO_DEPRECATED',
  '-DBOOST_ASIO_SEPARATE_COMPILATION',
  '-DBOOST_BEAST_SEPARATE_COMPILATION',
  '-DBOOST_EXCEPTION_DISABLE',
  '-DBOOST_NO_EXCEPTIONS',
  '-DBOOST_URL_NO_SOURCE_LOCATION',
  '-DJSON_NOEXCEPTION',
  '-DOPENSSL_NO_FILENAMES',
  '-DSDBUSPLUS_DISABLE_BOOST_COROUTINES',
]),
language : 'cpp')

boost_dep = dependency(
  'boost',
  modules: [
    'url',
    ],
  #version : '>=1.83.0',
  required : true,
  include_type: 'system'
)

#benchmark = subproject('benchmark')
nghttp2 = dependency('libnghttp2', version: '>=1.52.0', required : false)
if not nghttp2.found()
  cmake = import('cmake')
  opt_var = cmake.subproject_options()
  opt_var.add_cmake_defines({
    'ENABLE_LIB_ONLY': true,
    'ENABLE_STATIC_LIB': true
  })
  nghttp2_ex = cmake.subproject('nghttp2', options: opt_var)
  nghttp2 = nghttp2_ex.dependency('nghttp2')
endif
crow_core_dependencies = nghttp2


core_includes = include_directories(['.','./include','./http','./utils','./redfish_event/http','./redfish_event'])
if get_option('enable-tests')
  subdir('tests')
endif
subdir('redfish_event')


executable(
  'crow_core',
  [
    'plugintest.cpp',
    'boost_asio_ssl.cpp',
    'boost_asio.cpp',
    'boost_beast.cpp'
  #'reactortest.cpp'
  ],
  include_directories: core_includes,
  dependencies:[boost_dep,json_dep,openssl_dep,crow_core_dependencies],
  install: true,
  install_dir: get_option('bindir'),cpp_args : CROW_ARGS)