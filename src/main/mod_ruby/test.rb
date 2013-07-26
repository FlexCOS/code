#!/usr/bin/env ruby
require_relative "FlexCOS"

puts "Generate FlexCOS object"


def print_rapdu(rapdu)
  puts "RAPDU with %d bytes" % rapdu.size  

  rapdu.each_slice(16).each.with_index do |x, i|
    puts "%04d:\t" % (i*16+1) + x.map{ |x| "%02X" % x }. join(" ")
  end
end

print_rapdu(FlexCOS.send([80, 84, 00, 00, 04]))

