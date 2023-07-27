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
boost_dep = dependency('boost',modules: ['coroutine'])
json_dep = dependency('nlohmann_json')
gtest = subproject('gtest')
#benchmark = subproject('benchmark')

#boost_lib_path = boost_dep.get_pkgconfig_variable('libdir')
boost_lib_path='/opt/homebrew/Cellar/boost/1.82.0/lib'
core_includes = include_directories(['.','./include','./http'])
if get_option('enable-tests')
  subdir('tests')
endif
executable(
  'crow_core',
  ['crow_core.cpp','boost_url.cpp'],
  include_directories: core_includes,
  dependencies:[boost_dep,json_dep,openssl_dep],
  install: true,
  install_dir: get_option('bindir'),cpp_args : CROW_ARGS,link_args: ['-L' + boost_lib_path, '-lboost_coroutine'])