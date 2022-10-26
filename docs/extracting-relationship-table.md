# Extracting the Opposing Force relationship table

Opposing Force adds 2 new entity classifications, which means there are relationships we need to extract.

To do so, we'll use the Linux version, as we can run code in GDB.

```
cd <path_to_Half-Life_directory>
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<path_to_Half-Life_directory>
export LD_LIBRARY_PATH

gdb

file hl_linux
set args -game gearbox

run

#Start a map so the server dll is loaded

set logging file log.txt
set logging on

source gdb_script.gs
```

The GDB script contains the following:
```
set $i = 0
while($i < 16)
	set $j = 0
	while($j < 16)
		printf "\t%d, ", *( &'CBaseMonster::IRelationship(CBaseEntity*)::iEnemy' + $i * 16 + $j )
		set $j = $j + 1
	end
	printf "\n"
	set $i = $i + 1
end
```

Now all we need to do is take the last 2 columns and rows and add them to the existing table, and verify that the existing entries are correct.
