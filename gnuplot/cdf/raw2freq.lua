#!/usr/bin/env lua

if table.getn(arg) < 2 then
	io.write("Convert raw data to frequency aggregated by interval\n")
	io.write("Usage: raw2freq.lua <data_file> <aggr_interval>\n")
	do return end
end

local f = assert(io.open(arg[1], "r"))
local interval = tonumber(arg[2])
local total = 0

local data = {}
setmetatable(data, {__index = function () return 0 end})

while true do
	local line, _ = f:read()
	if not line then break end

	local index = math.floor(tonumber(line)/interval)
	data[index] = data[index] + 1
	total = total + 1
end

f:close()

for i=0,table.getn(data) do
-- for i, v in pairs(data) do
-- The trick here is that index 0 is special in lua
	data[i] = data[i] / total
	print(i * interval, data[i])
end
