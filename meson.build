project(
  'crow_core',
  'cpp',
  version: '0.1',
  meson_version: '>=0.58.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ])

CROW_ARGS=['-DBMCWEB_ENABLE_SSL','-DBMCWEB_INSECURE_DISABLE_AUTHX','-DBOOST_URL_NO_SOURCE_LOCATION','-DBMCWEB_INSECURE_DISABLE_AUTHX']
#CHAI_ARGS=[]
openssl_dep = dependency('openssl', version: '>=1.1.1')
boost_dep = dependency('boost',modules: ['coroutine','url'])
json_dep = dependency('nlohmann_json')
gtest = subproject('gtest')
reactor=subproject('reactor')
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

#boost_lib_path = boost_dep.get_pkgconfig_variable('libdir')
boost_lib_path='/opt/homebrew/Cellar/boost/1.82.0/lib'
core_includes = include_directories(['.','./include','./http','./utils'])
if get_option('enable-tests')
  subdir('tests')
endif
subdir('redfish_event')
reactor =subproject('reactor')
reactor_dep=reactor.get_variable('reactor_dep')
executable(
  'crow_core',
  ['plugintest.cpp'],
  include_directories: core_includes,
  dependencies:[boost_dep,json_dep,openssl_dep,crow_core_dependencies,reactor_dep],
  install: true,
  install_dir: get_option('bindir'),cpp_args : CROW_ARGS,link_args: ['-L' + boost_lib_path])