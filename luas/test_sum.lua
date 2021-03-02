
local sum = 0
print("initial sum=",sum)
for i=1, 100 do
    if i % 2 ==0 then 
        sum  = sum + i
    end
    --sum = sum +i 
end
print("after loop sum=",sum)

