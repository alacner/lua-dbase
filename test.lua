require"dbase"
require"print_r"
print(dbase.version())
local dbh,err = dbase.open("/tmp/base/shbase.dbf", 0)
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

--local r = dbh:get_record(9)
--print_r(r) 
--[[
for i=0,nr do
	local r = dbh:get_record_with_names(i)
	--print_r(r) 
end
--]]
dbh:close()
