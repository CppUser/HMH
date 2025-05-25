### Resource handling
 Things acuqired and released in   aggregate. instead of thinking about objects individually ,always think as part of group that handles together. Think about releasing in waves

### static
local_persist - lives inside the scope
global - accessed globaly
internal - 
static variables is initialized to 0 when created


###Allocating buck buffer 


### Unaligned accessing
on 64 bit architecture there is penalty for unaligned memory allocation
so for example if you operate on 32bit values it must be aligned with 32bit boundaries so it means it must start with 0 bit 4 - 8 bite ets , they should not start on 2 byte they always must be on multiples of 4 in memory
CPU setup to deal with those values that way

SO in current implementation of ResizeDIBSection we making sure that we touch pixel on 4byte boundry
so if we asking RGBA it means we asking R - 8 Bits G - 8Bits B-8Bits A-8bits that why its asked for 32bit that A is simply padding (extra bits to allign 4 bits structure)

RGB - 24bits
24bits - 3 bytes its not aligned with 4 bits so we have to go 32

##Animating backbuffer