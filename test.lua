require"dbase"
require"print_r"
print(dbase.version())
local dbh,err = dbase.open("/tmp/base/shbase.dbf", 2)
print(dbh)
print(err)
local nf = dbh:numfields()
print(nf)
local nr = dbh:numrecords()
print(nr)


local r = dbh:get_header_info()
print_r(r) 
local r = dbh:get_record_with_names(9)
print_r(r) 
local r = dbh:get_record(9)
print_r(r) 

local r,a,b,c = dbh:add_record {
    [1] = 100196,
    [2] = 'XX',
    [3] = '20031117',
    [4] = 'd',
    [5] = ''  ,      
    [6] = 0,
    [7] = 0,
    [8] = 0,
    [9] = 0,
    [10] = 0,
    [11] = 0,
    [12] = 0,
    [13] = 0,
    [14] = 0,
    [15] = 0,
    [16] = 0,
    [17] = 0,
    [18] = 0,
    [19] = 0,
    [20] = 0,
    [21] = 0,
    [22] = 0,
    [23] = 0,
    [24] = 0,
    [25] = 0,
    [26] = 0,
    [27] = 0,
    [28] = 0,
    [29] = 0,
    [30] = 0,
    [31] = 0,
    [32] = 0,
    [33] = 0,
    [34] = 20051028,
    [35] = 0,
    [36] = 0,
    [37] = 0,
    [38] = -1,
    [39] = 1.8,
    [40] = '' ,                
    [41] = 20090828,
    [42] = 0,
    [43] = 0,
    [44] = 0,
    [45] = 20090828012222,
    ['deleted'] = 0,
}
local r,a,b,c = dbh:add_record {
    100196,
    'XX',
    '20031117',
    'd',
    ''  ,      
    0,
    0,
    0,
    0,
    0,
    0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     20051028,
     0,
     0,
     0,
     -1,
     1.8,
     '' ,                
     20090828,
     0,
     0,
     0,
     20090828012222,
    ['deleted'] = 0,
}
print("---------------------")
print_r(r)
print_r(a)
print_r(b)
print_r(c)
--local r = dbh:get_record(9)
--print_r(r) 
--[[
for i=0,nr do
	local r = dbh:get_record_with_names(i)
	--print_r(r) 
end
--]]
dbh:close()
