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
local r,a,b,c = dbh:replace_record ({
    100196,
    'XX3',
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
	os.time(),
    ['deleted'] = 0,
}, 3)
print("---------------------")
print("---------------------")
local def = {
{"date", "d"},
{"name", "C", 50},
{"age", "N", 3, 0},
{"email", "C", 128},
{"ismember", "L"},
}
local dbh,a,b,c = dbase.create('/tmp/base/test.dbf', def)
dbh:add_record{os.time(), "alacner", 18, 'alacner@gmail.com', 1}
dbh:add_record{os.time(), "wgj", 28, 'w@g.j', 0}
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
