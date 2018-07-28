values = [[], [], []]
8192.times do |n|
  angle = (Math::PI * 2 * n) / 8192
  v1 = Math.sin(angle)
  v2 = Math.sin(angle + (Math::PI * 2) / 3)
  v3 = Math.sin(angle + (Math::PI * 4) / 3)
  min = [v1,v2,v3].min
  values[0] << (v1 - min) / 1.732050808
  values[1] << (v2 - min) / 1.732050808
  values[2] << (v3 - min) / 1.732050808
end

f = File.open('table.h', 'w')

f.write("static float table1[] = { ")
f.write(values[0].join(", "))
f.write(" };\n")

f.write("static float table2[] = { ")
f.write(values[1].join(", "))
f.write(" };\n")

f.write("static float table3[] = { ")
f.write(values[2].join(", "))
f.write(" };\n")
