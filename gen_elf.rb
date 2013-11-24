code = File.read('elsf.c')
fmt = File.read('elf.txt')

#prefix = code[/.*\n/ms]
prefix = code[/^/ms]
code = $'

ci = 0
fmt.size.times{|fi|
  if fmt[fi] == ?#
    while code[ci] == "\n"
      ci += 1
    end
    c = code[ci] || ';'
    fmt[fi] = c
    ci += 1
  end
}

File.open('elf.c', 'w') do |of|
  of.print prefix
  of.print fmt.gsub(/ +$/, '')
end
