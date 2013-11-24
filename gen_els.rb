#!/usr/bin/env ruby

if ARGV[0] == '-e'
  $strip_error = true
end

system("#{ENV['HOME']}/wrk/caddy/caddy.rb -sq elg.c")
c = File.read('out.c')

c.gsub!('}#', "}\n#")

#c.gsub!(/#define.*?\},/, "\n\\&\n")
#c.gsub!(/#define.*?abort\(\)/, "\\&\n")

alphas = [*'A'..'Z'] - ['A', 'E', 'F', 'T', 'U', 'D', 'I']
%w(ph paddr pfsize psize pbias
   g_argc g_argv argc argv
   addr sname val pagesize
   Hmmap Hdlsym Hlseek H__libc_start_main
).sort.uniq.each do |ident|
  al = alphas.pop
  c.gsub!(/\b#{ident}\b/, al)
end

if $strip_error
  c.sub!(/#define e.*\n/, '')
  c.sub!(/e\(([A-Z]\(.*?)==MAP_FAILED,"mmap"\),/, '\1,')
  c.gsub!(/\be\(.*?\);/, '')
end

File.open('els.c', 'w') do |of|
  of.print(c)
end
p alphas
puts c.size
