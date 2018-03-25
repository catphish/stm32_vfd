values = []
1024.times do |n|
  values << (Math.sin(Math::PI/2-Math::PI/6 + Math::PI/6/512*n)*1024).round
end

f = File.open('table.h', 'w')
f.write("static uint16_t table[] = { ")
f.write(values.join(", "))
f.write(" };\n")
