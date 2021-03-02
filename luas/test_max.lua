


local function max(...)
    local args = {...}
    --local val, idx = 0, 0
    local val,idx
    for i=1,#args do
        if val == nil or args[i]>val then 
            val, idx = args[i], i
        end
    end
    --print(val, idx)
    return val,idx 
end

v1 = max(1,2,3)
print(v1)
v2,i2 = max(100,9,8,2)
print(v2,i2)   ---- ->  100, 1
print(max(-11,9999,10))   --> 9999 2 


t = {max(17,28,101)} 
print(t[1],t[2]) ---> 101, 3
