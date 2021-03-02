
--这个文件用来测试简单的算术运算以及concate和len

--test add 
local a = 1
local b = "2.0"
local c = "3.0"
local d = 4.0

print(a)
print(b)
print(c)
print(d)

print("1 + '2.0'=", (a+b))
print("length of b is",#b)
print("length of c is",#c)
print("concate of bc is",b..c)
print("concate of abcd is:")
print(a..b..c..d)