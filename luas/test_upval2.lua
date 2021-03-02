
function newCounter()
    local count = 0
    local c2 = 1
    return function ()
        count = count + 1
        c2 = c2 *2
        return count, c2
    end
end

c1 = newCounter()
--local i = c1()
print(c1())
print(c1())
print(c1())
print(c1())
print(c1())




print("c2 is a new counter")

c2 = newCounter()
print(c2())
print(c2())
print(c2())

print("Now enter c1")
print(c1())
print(c1())
