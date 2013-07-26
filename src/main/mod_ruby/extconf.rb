require 'mkmf'

INCLUDE_DIR = RbConfig::CONFIG['includedir']

RbConfig::CONFIG.each do |k, v|
  puts "#{k} => #{v}"
end

HEADER_DIRS = [
  INCLUDE_DIR,
  '/usr/include/ruby-1.9.1/',
  '/usr/include/ruby-1.9.1/x86_64-linux/',
  '/home/covin/code/flexcos/code/src/main/common/',
  '/home/covin/code/flexcos/code/src/main/config/',
  '/home/covin/code/flexcos/code/src/main/core/',
  '/usr/include/'
]

puts HEADER_DIRS.map { |x| "Include directory: #{x}"}

dir_config('FlexCOS', HEADER_DIRS)

$defs.push("-D__USE_XIL_TYPES=0")

find_header('const.h', '../config')
find_header('array.h', '../common')
find_header('flexcos.h', '../core')

find_library('flexcos', nil)

create_makefile("FlexCOS")
